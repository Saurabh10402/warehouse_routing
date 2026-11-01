# Delayed-control design declaration

Read checkpoint 012 state, next steps and session 009; Git was clean and checkpoint_012 verification PASS. Inspected native table rebuild/event scheduling, strict schema parser and independent audit entry points. No simulator changes or new measurements in this step.

Created docs/ns3-detection-contract.md: three independent states, captured installation snapshots, deterministic stage ordering, stale preferred-service behavior, short-flap semantics, cutoff handling and 32 new controlled cases across four policies. The source-attempt check stays physical; destination selection stays installed. These distinctions prevent instantaneous failover from leaking into the delayed model.

Next exact action: implement nativeControl parsing with core rejection, physical/detected/installed snapshots and control_updates.csv; extend the independent audit and controlled tests together. Execute the declared validation before marking support complete. Existing checkpoint 012 raw results and archives are unchanged.
