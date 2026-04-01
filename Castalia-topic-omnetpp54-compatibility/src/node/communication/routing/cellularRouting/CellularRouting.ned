package node.communication.routing.cellularRouting;

simple CellularRouting like node.communication.routing.iRouting {

    parameters:
        bool collectTraceInfo = default(false);
        int maxNetFrameSize = default(0);         
        int netDataFrameOverhead = default(12);   
        int netBufferSize = default(32);  

        bool isSink = default(false); 

        int numberHelloIntervals = default(6);
        int sensingDuration = default(7000);
        int clCalculationTime = default(300);
        int routingTableUpdateTime = default(100); 
        int state1Time = default(1000);
        int sensingStageTime = default(3000);
        int reconfigurationTime = default(4000);
        int clConfirmationTime = default(100);
        int maxNeighborNumber = default(500);
        int maxHopCount = default(100);

        double colorTimeSlot = default (600);   

        int sensorDataDub = default (1);     

        int gridOffset = default(100);
        double helloInterval = default(10);   
        double clElectionDelayInterval = default(100);  
        double cellRadius = default(80);       
        bool isCH = default(false);              
        
        @display("i=block/routing");

    gates:
	output toCommunicationModule;
	output toMacModule;
	input fromCommunicationModule;
	input fromMacModule;
	input fromCommModuleResourceMgr;
}
