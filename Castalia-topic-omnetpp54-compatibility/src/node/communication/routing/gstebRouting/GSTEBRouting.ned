package node.communication.routing.gstebRouting;

simple GSTEBRouting like node.communication.routing.iRouting

{
 parameters:
	bool collectTraceInfo = default (false);
	int maxNetFrameSize = default (0);	// bytes
	int netDataFrameOverhead = default (10);	// bytes
	int netBufferSize = default (32);	// number of messages
	
	bool isCH = default(false);
	double alpha = default(0.01);

 gates:
	output toCommunicationModule;
	output toMacModule;
	input fromCommunicationModule;
	input fromMacModule;
	input fromCommModuleResourceMgr;
}

