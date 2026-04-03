//****************************************************************************
//*  Copyright: National ICT Australia,  2007 - 2012                         *
//*  Developed at the ATP lab, Networked Systems research theme              *
//*  Author(s): Athanassios Boulis, Yuriy Tselishchev                        *
//*  This file is distributed under the terms in the attached LICENSE file.  *
//*  If you do not find this file, copies can be found by writing to:        *
//*                                                                          *
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia             *
//*      Attention:  License Inquiry.                                        *
//*                                                                          *  
//****************************************************************************/

import node.communication.mac.MacPacket;

// NS3 CC2420 MAPPING: 802.15.4 MAC packet types
// Maps to: Cc2420Header::FrameType or Cc2420Mac::MacFrameType
// v1.0 focus: DATA + ACK; BEACON/GTS are optional/deferred.
enum Mac802154Packet_type {
	MAC_802154_BEACON_PACKET = 1001;
	MAC_802154_ASSOCIATE_PACKET = 1002;
	MAC_802154_DATA_PACKET = 1003;
	MAC_802154_ACK_PACKET = 1004;
	MAC_802154_GTS_REQUEST_PACKET = 1005;
}

// NS3 CC2420 MAPPING: GTS descriptor (optional in v1.0)
// Used only for beacon/GTS scheduling; can be stubbed if GTS not implemented.
struct Basic802154GTSspec {
	int owner;
	int start;
	int length;
}

// NS3 CC2420 MAPPING: 802.15.4 MAC frame format
// Maps to: Cc2420Header/Cc2420Trailer fields (PAN ID, src/dst, sequence)
// RSSI/LQI populated by PHY on receive (via MacPacket base fields).
packet Basic802154Packet extends MacPacket {
	int Mac802154PacketType enum (Mac802154Packet_type);
	int PANid;
	int srcID;
	int dstID;
	int seqNum;

	// those fields belong to beacon packet (MAC_802154_BEACON_PACKET)
	int beaconOrder;
	int frameOrder;
	int BSN;
	int CAPlength;
	int GTSlength;
	Basic802154GTSspec GTSlist[];
}

