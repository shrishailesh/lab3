/*use this for lab3. Commit and push such that no conflicts are generated.
In case of conflicts, please roll back and resolve conficts.
*/

/*Some common header files*/

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/olsr-module.h"
#include "ns3/aodv-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/traced-value.h"
#include "ns3/queue.h"
#include "ns3/trace-source-accessor.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream.h>
#include <math.h>
#include <string.h>

using namespace ns3;

//Setting up global variable for keeping a track of transmitted bytes and received bytes

uint32_t transmitted_bytes = 0;	// sent bytes
uint32_t received_bytes = 0;	// received bytes

//A simple method to update the transmitted bytes
void Trace(Ptr<const Packet> packet_value) {
	transmitted_bytes += packet_value->GetSize();
}

int main(int argc, char* argv[]) {
    SeedManager::SetSeed(11223344); //Keep a common seed to compare results
    uint32_t num_nodes = 20; 
    double intensity = 0.1;
    double power = 1.0; //corresponds to 1mW
    uint32_t area = 1000; //Area of 1000*1000.
    uint32_t area_sq = area*area;
    double node_density = num_nodes/area_sq; //default value is 20/1000000
    double tx_rate;
    double efficiency;
    
    //Size of the area in String to easily pass to functions
    String area_str = "1000";
    String route_prot = "OLSR";
    
    CommandLine cmd;
    cmd.AddValue("num_nodes", "Number of nodes in the simulation", num_nodes);
    cmd.AddValue("intensity", "Intensity varying between 0.1 and 0.9", intensity);
    cmd.AddValue("power", "Transmission power in mW", power);
    cmd.AddValue("route_prot", "Routing Protocol: AODV or OLSR", route_prot);
    cmd.Parse (argc, argv);
}
