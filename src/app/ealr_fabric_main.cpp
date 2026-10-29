#include "ealr/metrics/fabric_metrics.hpp"
#include <filesystem>
#include <iostream>
#include <string>

int main(int argc,char** argv) {
    try {
        std::filesystem::path config,output;
        for(int i=1;i<argc;++i) {
            std::string arg=argv[i];
            if((arg=="--config"||arg=="--output")&&i+1<argc) {
                if(arg=="--config")config=argv[++i];else output=argv[++i];
            } else throw std::invalid_argument("Usage: ealr-fabric --config scenario.json --output NEW_DIRECTORY");
        }
        if(config.empty()||output.empty())throw std::invalid_argument("Both --config and --output required");
        if(std::filesystem::exists(output))throw std::invalid_argument("Output exists; choose a new directory");
        auto c=ealr::fabric::load(config);auto result=ealr::fabric::simulate(c);
        ealr::fabric::writeResults(c,result,output);
        std::filesystem::copy_file(config,output/"config.json");
        auto metrics=ealr::fabric::summarize(c,result);
        std::cout<<"Core abstract-link simulation: "<<metrics.delivered<<'/'<<metrics.sent<<" delivered; output "<<output<<'\n';
        return 0;
    }catch(const std::exception& error){std::cerr<<"Experiment failed: "<<error.what()<<'\n';return 1;}
}
