// SPDX-License-Identifier: GPL-2.0-only
// Behavioral authorization model: no cryptography or real main-server internals.
#include <sstream>
#include <iomanip>

struct TaskControl
{
    struct Task
    {
        std::string source,destination,state="created";
        bool protectedTask=false;
        double deadlineMs=0,startMs=0;
        Ptr<Socket> client,sink;
        Time start,approved,sent,received;
        uint32_t tx=0,rx=0;
    };
    std::vector<Task> tasks;
    double upperDelayMs=0,processingMs=0,timeoutMs=0,stopSeconds=0;
    std::string response,outageSpine,routingPolicy;
    double downMs=0,upMs=0;
    Ipv4Address serverAddress;
    Ptr<Socket> serverSocket;
    std::map<std::string,Ipv4Address> addresses;

    void Event(size_t i,const std::string& event)
    {
        std::cout << "TASK_EVENT " << i << " " << event << " " << Simulator::Now().GetNanoSeconds() << '\n';
    }
    static std::string Text(Ptr<Packet> packet)
    {
        std::string value(packet->GetSize(),'\0');
        packet->CopyData(reinterpret_cast<uint8_t*>(value.data()),value.size());
        return value;
    }
    static Ptr<Packet> Message(const std::string& value)
    {
        return Create<Packet>(reinterpret_cast<const uint8_t*>(value.data()),value.size());
    }
    void Load(const std::string& file,uint32_t taskLimit=1000)
    {
        std::ifstream in(file);
        NS_ABORT_MSG_IF(!(in>>upperDelayMs>>processingMs>>timeoutMs>>response>>outageSpine>>downMs>>upMs>>routingPolicy),"Invalid service table");
        NS_ABORT_MSG_IF(upperDelayMs<=0 || processingMs<=0 || timeoutMs<=0 || timeoutMs>60000,"Invalid control timing");
        NS_ABORT_MSG_IF(response!="allow" && response!="deny" && response!="timeout","Invalid response mode");
        Task t; int required;
        NS_ABORT_MSG_IF(taskLimit!=1000 && taskLimit!=2000,"Invalid task limit");
        while(in>>t.source>>t.destination>>required>>t.deadlineMs>>t.startMs)
        {
            NS_ABORT_MSG_IF((required!=0 && required!=1) || t.deadlineMs<=0 || t.startMs<1000 || t.startMs>60000,"Invalid task");
            t.protectedTask=required;tasks.push_back(t);
            NS_ABORT_MSG_IF(tasks.size()>taskLimit,"Task limit exceeded");
        }
        NS_ABORT_MSG_IF(!in.eof() || tasks.empty(),"Invalid task table");
        double lastMs=upMs;
        for(const auto& task:tasks)lastMs=std::max(lastMs,task.startMs);
        stopSeconds=lastMs/1000+2.0+timeoutMs/1000+2*upperDelayMs/1000+processingMs/1000;
    }
    void Install(std::map<std::string,Entry>& nodes,const std::map<std::string,Ipv4Address>& ips,Ptr<Node> server)
    {
        addresses=ips;serverAddress=ips.at("main-server-model");
        serverSocket=Socket::CreateSocket(server,UdpSocketFactory::GetTypeId());
        // Replies must retain the advertised identity on either uplink.
        NS_ABORT_MSG_IF(serverSocket->Bind(InetSocketAddress(serverAddress,8000))<0,"Server bind failed");
        serverSocket->SetRecvCallback(MakeCallback(&TaskControl::ServerReceive,this));
        for(size_t i=0;i<tasks.size();++i)
        {
            auto& t=tasks[i];
            NS_ABORT_MSG_IF(!nodes.count(t.source) || !nodes.count(t.destination),"Unknown task robot");
            t.client=Socket::CreateSocket(nodes.at(t.source).node,UdpSocketFactory::GetTypeId());
            NS_ABORT_MSG_IF(t.client->Bind(InetSocketAddress(addresses.at(t.source),10000+i))<0,"Client bind failed");
            t.client->SetRecvCallback(MakeCallback(&TaskControl::ClientReceive,this));
            t.sink=Socket::CreateSocket(nodes.at(t.destination).node,UdpSocketFactory::GetTypeId());
            NS_ABORT_MSG_IF(t.sink->Bind(InetSocketAddress(Ipv4Address::GetAny(),20000+i))<0,"Sink bind failed");
            t.sink->SetRecvCallback(MakeCallback(&TaskControl::DataReceive,this));
            Simulator::Schedule(ControlMs(t.startMs),&TaskControl::Start,this,i);
        }
    }
    void Start(size_t i)
    {
        auto& t=tasks[i]; t.start=Simulator::Now();Event(i,"start");
        if(!t.protectedTask) {t.state="ready";SendData(i);return;}
        t.state="waiting";Event(i,"auth-request");
        t.client->SendTo(Message("REQ "+std::to_string(i)),0,InetSocketAddress(serverAddress,8000));
        Simulator::Schedule(ControlMs(timeoutMs),&TaskControl::Expire,this,i);
    }
    void Expire(size_t i)
    {
        if(tasks[i].state=="waiting") {tasks[i].state="timeout";Event(i,"timeout");}
    }
    void ServerReceive(Ptr<Socket> socket)
    {
        Address sender;
        while(auto packet=socket->RecvFrom(sender))
        {
            std::istringstream in(Text(packet)); std::string kind; size_t i;
            if(!(in>>kind>>i) || kind!="REQ" || i>=tasks.size() || !InetSocketAddress::IsMatchingType(sender))continue;
            auto addr=InetSocketAddress::ConvertFrom(sender);
            if(!tasks[i].protectedTask || addr.GetIpv4()!=addresses.at(tasks[i].source) || addr.GetPort()!=10000+i)continue;
            Event(i,"server-request");
            if(response!="timeout")Simulator::Schedule(ControlMs(processingMs),&TaskControl::Reply,this,i,sender);
        }
    }
    void Reply(size_t i,Address sender)
    {
        Event(i,"server-"+response);
        serverSocket->SendTo(Message(response+" "+std::to_string(i)),0,sender);
    }
    void ClientReceive(Ptr<Socket> socket)
    {
        Address sender;
        while(auto packet=socket->RecvFrom(sender))
        {
            std::istringstream in(Text(packet));std::string answer;size_t i;
            if(!(in>>answer>>i) || i>=tasks.size() || tasks[i].client!=socket || !InetSocketAddress::IsMatchingType(sender))continue;
            auto addr=InetSocketAddress::ConvertFrom(sender);
            if(addr.GetIpv4()!=serverAddress || addr.GetPort()!=8000)continue;
            auto& t=tasks[i];
            if(t.state!="waiting" || Simulator::Now()>=t.start+ControlMs(timeoutMs)) {Event(i,"ignored-response");continue;}
            if(answer=="allow") {t.state="ready";t.approved=Simulator::Now();Event(i,"approved");SendData(i);}
            else if(answer=="deny") {t.state="denied";Event(i,"denied");}
        }
    }
    void SendData(size_t i)
    {
        auto& t=tasks[i];NS_ABORT_MSG_IF(t.state!="ready","Task bypassed authorization gate");
        t.state="sent";t.sent=Simulator::Now();Event(i,"data-tx");
        std::string data="DATA "+std::to_string(i);data.resize(256,' ');
        if(t.client->SendTo(Message(data),0,InetSocketAddress(addresses.at(t.destination),20000+i))==256)t.tx++;
    }
    void DataReceive(Ptr<Socket> socket)
    {
        Address sender;
        while(auto packet=socket->RecvFrom(sender))
        {
            std::istringstream in(Text(packet));std::string kind;size_t i;
            if(!(in>>kind>>i) || kind!="DATA" || i>=tasks.size() || tasks[i].sink!=socket)continue;
            auto& t=tasks[i];
            NS_ABORT_MSG_IF(t.state!="sent","Payload arrived for blocked/duplicate task");
            t.state="delivered";t.rx++;t.received=Simulator::Now();Event(i,"data-rx");
        }
    }
    bool Report()
    {
        bool pass=true;
        for(size_t i=0;i<tasks.size();++i)
        {
            const auto& t=tasks[i];
            auto elapsed=t.rx ? (t.received-t.start).GetNanoSeconds() : -1;
            auto payload=t.rx ? (t.received-t.sent).GetNanoSeconds() : -1;
            bool met=t.rx && elapsed<=ControlMs(t.deadlineMs).GetNanoSeconds();
            std::cout << "TASK_RESULT " << i << " " << t.state << " " << t.tx << " " << t.rx << " " << elapsed << " " << payload << " " << met << '\n';
            if(!t.protectedTask || response=="allow")pass=pass && t.rx==1;
            else pass=pass && t.tx==0 && t.rx==0 && t.state==(response=="deny" ? "denied" : "timeout");
        }
        return pass;
    }
};
