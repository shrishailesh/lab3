/*use this for lab3. Commit and push such that no conflicts are generated.
In case of conflicts, please roll back and resolve conficts.
*/

/*Some common header files*/

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/applications-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/olsr-module.h"
#include "ns3/aodv-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/traced-value.h"
#include "ns3/queue.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/position-allocator.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <math.h>
#include <string>

using namespace ns3;
using namespace std;

//Setting up global variable for keeping a track of transmitted bytes and received bytes

uint32_t transmitted_bytes = 0;
uint32_t received_bytes = 0;

//A simple method to update the transmitted bytes
void Trace(Ptr<const Packet> packet_value) {
	transmitted_bytes += packet_value->GetSize();
}

int main(int argc, char* argv[]) {
    SeedManager::SetSeed(11223344); //Keep a common seed to compare results
    uint32_t num_nodes = 20; 
    double intensity = 0.1; // Traffic intensity
    double power = 1.0; //corresponds to 1mW
    uint32_t dimension = 1000; //Area of 1000*1000.
    uint32_t area_sq = dimension*dimension;
    double node_density = (float)num_nodes/area_sq; // node density
    double tx_rate;
    double max_rate = 11000000; // Max data rate of 11 Mbps
    double efficiency;
    
    //Size of the area in String to easily pass to functions
    string Mode ("DsssRate1Mbps");    
    string dim_str = "1000";
    string route_prot = "OLSR";
    
    Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200")); // Fragmentation disabled for frames below 2200
    Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",StringValue (Mode)); // data rate of non-unicast same as unicast

    CommandLine cmd;
    cmd.AddValue("num_nodes", "Number of nodes in the simulation", num_nodes);
    cmd.AddValue("intensity", "Intensity varying between 0.1 and 0.9", intensity);
    cmd.AddValue("power", "Transmission power in mW", power);
    cmd.AddValue("route_prot", "Routing Protocol: AODV or OLSR", route_prot);
    cmd.AddValue("dimension", "Dimensions of the field area", dimension);
    cmd.Parse (argc, argv);

    Ptr<UniformRandomVariable> peer = CreateObject<UniformRandomVariable> (); // Random peers
    Ptr<UniformRandomVariable> start = CreateObject<UniformRandomVariable> (); // Random start

    start->SetAttribute("Min", DoubleValue(0.0));
    start->SetAttribute("Max", DoubleValue(1.0));
    peer->SetAttribute("Min", DoubleValue(0));
    peer->SetAttribute("Max", DoubleValue(num_nodes-1));

    tx_rate = (intensity * max_rate)/num_nodes; // Data rate of each node

    double rand_start[num_nodes];
    uint32_t rand_peers[num_nodes];

    // Generating network nodes
    NodeContainer nodes;
    nodes.Create(num_nodes);

    // Placement of nodes in the given area
    MobilityHelper mobile;
    string position = "ns3::UniformRandomVariable[Min=0.0|Max="+dim_str+"]";
    mobile.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
    "X", StringValue (position),
    "Y", StringValue (position));
    mobile.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobile.Install(nodes);

    // Wireless phy and MAC
    WifiHelper wireless;
    double dBMpower = 10 * log10 (power); // Converting power to dBM
    YansWifiPhyHelper physical = YansWifiPhyHelper::Default(); // Physical layer model
    physical.Set ("RxGain", DoubleValue(0));
    physical.Set ("TxPowerStart", DoubleValue(dBMpower));
    physical.Set ("TxPowerEnd", DoubleValue(dBMpower));

    YansWifiChannelHelper channel;
    channel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
    channel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
    physical.SetChannel (channel.Create());

    NqosWifiMacHelper wirelessMac = NqosWifiMacHelper::Default();
    wireless.SetStandard (WIFI_PHY_STANDARD_80211b);
    wireless.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
    "DataMode",StringValue (Mode),
    "ControlMode",StringValue (Mode));

    wirelessMac.SetType ("ns3::AdhocWifiMac");
    NetDeviceContainer WirelessDevice = wireless.Install (physical, wirelessMac, nodes);

    cout << "MAC and Phy done." << endl;

    // Set routing protocol
    Ipv4ListRoutingHelper route;
    InternetStackHelper stack;

    if(route_prot == "AODV")
    {
    	AodvHelper aodv;
        route.Add (aodv, 50);
	stack.SetRoutingHelper (route);
    }
    else if(route_prot == "OLSR")
    {
    	OlsrHelper olsr;
        route.Add (olsr, 50);
	stack.SetRoutingHelper (route);
    }
    stack.Install(nodes);

    cout << "Routing protocol set" << endl;

    // Generate IPv4 addresses
    Ipv4AddressHelper address;
    address.SetBase ("10.1.0.0", "255.255.0.0");
    Ipv4InterfaceContainer interfaces;
    interfaces = address.Assign (WirelessDevice);

    uint32_t port = 6000;
    ApplicationContainer sourceApp[num_nodes]; 
    ApplicationContainer sinkApp[num_nodes];

    //Set peers and install applications
    for(uint32_t i=0; i< num_nodes; i++)
    {
	rand_start[i] = start->GetValue();
	rand_peers[i] = peer->GetValue();
	
	OnOffHelper UDPHelper("ns3::UdpSocketFactory", Address());
	UDPHelper.SetConstantRate(DataRate(tx_rate));
	
	while(rand_peers[i] == i)
	{
	    rand_peers[i] = peer->GetValue();
	}
	AddressValue Addr (InetSocketAddress(interfaces.GetAddress(rand_peers[i]),port));
        UDPHelper.SetAttribute("Remote",Addr);
        
	sourceApp[i] = UDPHelper.Install(nodes.Get(i)); 
	sourceApp[i].Start(Seconds(rand_start[i]));
	sourceApp[i].Stop(Seconds (10.0));

	PacketSinkHelper UDPSink("ns3::UdpSocketFactory",InetSocketAddress(Ipv4Address::GetAny(),port));
	sinkApp[i]= UDPSink.Install(nodes.Get(i));
	sinkApp[i].Start(Seconds(rand_start[i]));
	sinkApp[i].Stop(Seconds(10.0));
	
	Ptr<OnOffApplication> source1 = DynamicCast<OnOffApplication> (sourceApp[i].Get(0));
	source1->TraceConnectWithoutContext ("Tx", MakeCallback(&Trace));   // Call back method to count transmitted bytes
    }

    cout << "Applications done." << endl;

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    // Generate animation
    AnimationInterface anime("p3_anim.xml");

    cout <<"Animation done." << endl;

    Simulator::Stop(Seconds(10.0));
    Simulator::Run ();

   cout << "Now for received bytes" << endl;

    Ptr<PacketSink> sink1;

    for(uint32_t i = 0;i < num_nodes; i++)
    {
	sink1 = DynamicCast<PacketSink> (sinkApp[i].Get(0));
	received_bytes += sink1->GetTotalRx();
    }

    efficiency = (double)received_bytes/(double)transmitted_bytes;
  
    cout<<"Nodes="<<num_nodes<<" Dimension="<<dimension<<" Intensity="<<intensity<<" Protocol="<<route_prot<<" PowerTx(mW)="<<power<<" Efficiency="<<efficiency<<" nodeDensity="<<node_density<<endl;

    Simulator::Destroy ();
    return 0;

}
