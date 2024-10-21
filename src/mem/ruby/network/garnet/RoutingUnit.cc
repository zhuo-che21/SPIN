/*
 * Copyright (c) 2008 Princeton University
 * Copyright (c) 2016 Georgia Institute of Technology
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <random>
#include "mem/ruby/network/garnet/RoutingUnit.hh"
#include <iostream>
#include "base/cast.hh"
#include "base/compiler.hh"
#include "debug/RubyNetwork.hh"
#include "mem/ruby/network/garnet/InputUnit.hh"
#include "mem/ruby/network/garnet/OutputUnit.hh"
#include "mem/ruby/network/garnet/Router.hh"
#include "mem/ruby/slicc_interface/Message.hh"

namespace gem5
{

namespace ruby
{

namespace garnet
{

RoutingUnit::RoutingUnit(Router *router)
{
    m_router = router;
    m_routing_table.clear();
    m_weight_table.clear();
}

void
RoutingUnit::addRoute(std::vector<NetDest>& routing_table_entry)
{
    if (routing_table_entry.size() > m_routing_table.size()) {
        m_routing_table.resize(routing_table_entry.size());
    }
    for (int v = 0; v < routing_table_entry.size(); v++) {
        m_routing_table[v].push_back(routing_table_entry[v]);
    }
}

void
RoutingUnit::addWeight(int link_weight)
{
    m_weight_table.push_back(link_weight);
}

bool
RoutingUnit::supportsVnet(int vnet, std::vector<int> sVnets)
{
    // If all vnets are supported, return true
    if (sVnets.size() == 0) {
        return true;
    }

    // Find the vnet in the vector, return true
    if (std::find(sVnets.begin(), sVnets.end(), vnet) != sVnets.end()) {
        return true;
    }

    // Not supported vnet
    return false;
}

/*
 * This is the default routing algorithm in garnet.
 * The routing table is populated during topology creation.
 * Routes can be biased via weight assignments in the topology file.
 * Correct weight assignments are critical to provide deadlock avoidance.
 */
int
RoutingUnit::lookupRoutingTable(int vnet, NetDest msg_destination)
{
    // First find all possible output link candidates
    // For ordered vnet, just choose the first
    // (to make sure different packets don't choose different routes)
    // For unordered vnet, randomly choose any of the links
    // To have a strict ordering between links, they should be given
    // different weights in the topology file

    int output_link = -1;
    int min_weight = INFINITE_;
    std::vector<int> output_link_candidates;
    int num_candidates = 0;

    // Identify the minimum weight among the candidate output links
    for (int link = 0; link < m_routing_table[vnet].size(); link++) {
        if (msg_destination.intersectionIsNotEmpty(
            m_routing_table[vnet][link])) {

        if (m_weight_table[link] <= min_weight)
            min_weight = m_weight_table[link];
        }
    }

    // Collect all candidate output links with this minimum weight
    for (int link = 0; link < m_routing_table[vnet].size(); link++) {
        if (msg_destination.intersectionIsNotEmpty(
            m_routing_table[vnet][link])) {

            if (m_weight_table[link] == min_weight) {
                num_candidates++;
                output_link_candidates.push_back(link);
            }
        }
    }

    if (output_link_candidates.size() == 0) {
        fatal("Fatal Error:: No Route exists from this Router.");
        exit(0);
    }

    // Randomly select any candidate output link
    int candidate = 0;
    if (!(m_router->get_net_ptr())->isVNetOrdered(vnet))
        candidate = rand() % num_candidates;

    output_link = output_link_candidates.at(candidate);
    return output_link;
}


void
RoutingUnit::addInDirection(PortDirection inport_dirn, int inport_idx)
{
    m_inports_dirn2idx[inport_dirn] = inport_idx;
    m_inports_idx2dirn[inport_idx]  = inport_dirn;
}

void
RoutingUnit::addOutDirection(PortDirection outport_dirn, int outport_idx)
{
    m_outports_dirn2idx[outport_dirn] = outport_idx;
    m_outports_idx2dirn[outport_idx]  = outport_dirn;
}

// outportCompute() is called by the InputUnit
// It calls the routing table by default.
// A template for adaptive topology-specific routing algorithm
// implementations using port directions rather than a static routing
// table is provided here.

int*
RoutingUnit::outportCompute_dyXYZ(RouteInfo route, int inport,
                            PortDirection inport_dirn)
{
    static int result[2];
    if (route.dest_router == m_router->get_id()) {

        // Multiple NIs may be connected to this router,
        // all with output port direction = "Local"
        // Get exact outport id from table
        int outport = lookupRoutingTable(route.vnet, route.net_dest);
        result[1] = outport;
        result[0] = -1;
        return result;
    }
    
    PortDirection outport_dirn = "Unknown";

    int num_rows = m_router->get_net_ptr()->getNumRows();
    int num_cols = m_router->get_net_ptr()->getNumCols();
    // int num_rout = m_router->get_net_ptr()->getNumRouters();
    assert(num_rows > 0 && num_cols > 0);

    int my_id = m_router->get_id();
    int my_x = my_id % num_cols;
    int my_y = (my_id % (num_cols * num_rows)) / num_cols;
    int my_z = my_id / (num_cols * num_rows);

    int dest_id = route.dest_router;
    int dest_x = dest_id % num_cols;
    int dest_y = (dest_id % (num_cols * num_rows)) / num_cols;
    int dest_z = dest_id / (num_cols * num_rows);

    int x_hops = abs(dest_x - my_x);
    int y_hops = abs(dest_y - my_y);
    int z_hops = abs(dest_z - my_z);

    bool x_dirn = (dest_x >= my_x);
    bool y_dirn = (dest_y >= my_y);
    bool z_dirn = (dest_z >= my_z);

    int exa_array[3];
    switch (int(x_dirn * 4 + y_dirn * 2 + z_dirn))
    {
    case 7:
        exa_array[0] = 2;
        exa_array[1] = 2;
        exa_array[2] = 3;
        break;
    case 6:
        exa_array[0] = 3;
        exa_array[1] = 3;
        exa_array[2] = 3;
        break;
    case 5:
        exa_array[0] = 0;
        exa_array[1] = 2;
        exa_array[2] = 2;
        break;
    case 4:
        exa_array[0] = 1;
        exa_array[1] = 3;
        exa_array[2] = 2;
        break;
    case 3:
        exa_array[0] = 2;
        exa_array[1] = 0;
        exa_array[2] = 1;
        break;
    case 2:
        exa_array[0] = 3;
        exa_array[1] = 1;
        exa_array[2] = 1;
        break;
    case 1:
        exa_array[0] = 0;
        exa_array[1] = 0;
        exa_array[2] = 0;
        break;
    case 0:
        exa_array[0] = 1;
        exa_array[1] = 1;
        exa_array[2] = 0;
        break;
    default:
        break;
    }

    int E_outport = m_outports_dirn2idx["East"];
    int W_outport = m_outports_dirn2idx["West"];
    int N_outport = m_outports_dirn2idx["North"];
    int S_outport = m_outports_dirn2idx["South"];
    int U_outport = m_outports_dirn2idx["Up"];
    int D_outport = m_outports_dirn2idx["Down"];
    // result[0] = exa_array[dir_ind];
    // std::cout<<m_exa_vc<<std::endl;
    // if (dir_ind == 0)
    // {
    //     if (x_dirn) {
    //         assert(inport_dirn == "Local" || inport_dirn == "West");
    //         outport_dirn = "East";
    //     } else {
    //         assert(inport_dirn == "Local" || inport_dirn == "East");
    //         outport_dirn = "West";
    //     }
    // } else if(dir_ind == 1)
    // {
    //     if (y_dirn) {
    //         outport_dirn = "North";
    //     } else {
    //         outport_dirn = "South";
    //     }
    // } else 
    // {
    //     if (z_dirn)
    //     {
    //         outport_dirn = "Down";
    //     } else{
    //         outport_dirn = "Up";
    //     }
    // }
    // int max_point = 0;
    // if ((x_hops > 0) && (m_router->getOutputUnit(E_outport)->get_point(exa_array[0])>max_point)) {
    //     result[0] = exa_array[0];
    //     max_point = m_router->getOutputUnit(E_outport)->get_point(exa_array[0]);
    //     if (x_dirn) {
    //         // assert(inport_dirn == "Local" || inport_dirn == "West");
    //         outport_dirn = "East";
    //     } else {
    //         // assert(inport_dirn == "Local" || inport_dirn == "East");
    //         outport_dirn = "West";
    //     }
    // } 
    // std::cout<<max_point<<std::endl;
    // if ((y_hops > 0) && (m_router->getOutputUnit(E_outport)->get_point(exa_array[1])>max_point)) {
    //     max_point = m_router->getOutputUnit(E_outport)->get_point(exa_array[1]);
    //     result[0] = exa_array[1];
    //     if (y_dirn) {
    //         outport_dirn = "North";
    //     } else {
    //         outport_dirn = "South";
    //     }
    // } 
    // std::cout<<max_point<<std::endl;
    // if ((z_hops > 0) && (m_router->getOutputUnit(E_outport)->get_point(exa_array[2])>max_point))
    // {
    //     max_point = m_router->getOutputUnit(E_outport)->get_point(exa_array[2]);
    //     result[0] = exa_array[2];
    //     if (z_dirn)
    //     {
    //         outport_dirn = "Down";
    //     } else{
    //         outport_dirn = "Up";
    //     }
    // }
    // std::cout<<max_point<<std::endl;
    int assign = 0;
    if (x_hops > 0) {
        result[0] = exa_array[0];
        if (x_dirn) {
            auto E_output_unit = m_router->getOutputUnit(E_outport);
            if (!E_output_unit->is_blocked(exa_array[0]))
            {
                assign = 1;
            }
            outport_dirn = "East";
        } else {
            auto W_output_unit = m_router->getOutputUnit(W_outport);
            if (!W_output_unit->is_blocked(exa_array[0]))
            {
                assign = 1;
            }
            outport_dirn = "West";
        }
    } 
    if ((y_hops > 0) && (assign == 0)) {
        result[0] = exa_array[1];
        if (y_dirn) {
            auto N_output_unit = m_router->getOutputUnit(N_outport);
            if (!N_output_unit->is_blocked(exa_array[1]))
            {
                assign = 1;
            }
            outport_dirn = "North";
        } else {
            auto S_output_unit = m_router->getOutputUnit(S_outport);
            if (!S_output_unit->is_blocked(exa_array[1]))
            {
                assign = 1;
            }
            outport_dirn = "South";
        }
    } 
    if ((z_hops > 0) && (assign == 0))
    {
        result[0] = exa_array[2];
        if (z_dirn)
        {
            auto D_output_unit = m_router->getOutputUnit(D_outport);
            if (!D_output_unit->is_blocked(exa_array[2]))
            {
                assign = 1;
            }
            outport_dirn = "Down";
        } else{
            auto U_output_unit = m_router->getOutputUnit(U_outport);
            if (!U_output_unit->is_blocked(exa_array[2]))
            {
                assign = 1;
            }
            outport_dirn = "Up";
        }
    }
    result[1] = m_outports_dirn2idx[outport_dirn];
    return result;
}

int*
RoutingUnit::outportCompute(RouteInfo route, int inport,
                            PortDirection inport_dirn)
{
    int outport = -1;
    static int result[2];
    if (route.dest_router == m_router->get_id()) {

        // Multiple NIs may be connected to this router,
        // all with output port direction = "Local"
        // Get exact outport id from table
        outport = lookupRoutingTable(route.vnet, route.net_dest);
        result[1] = outport;
        result[0] = -1;
        return result;
    }

    // Routing Algorithm set in GarnetNetwork.py
    // Can be over-ridden from command line using --routing-algorithm = 1
    RoutingAlgorithm routing_algorithm =
        (RoutingAlgorithm) m_router->get_net_ptr()->getRoutingAlgorithm();

    switch (routing_algorithm) {
        case TABLE_:  outport =
            lookupRoutingTable(route.vnet, route.net_dest); 
            result[1] = outport;
            result[0] = -1;
            break;
        case XY_:     outport =
            outportComputeXY(route, inport, inport_dirn); 
            result[1] = outport;
            result[0] = -1;
            break;
        // any custom algorithm
        case CUSTOM_: outport =
            outportComputeCustom(route, inport, inport_dirn);
            result[1] = outport;
            result[0] = -1;
            break;
        case NUM_ROUTING_ALGORITHM_:
            return outportCompute_dyXYZ(route, inport, inport_dirn);
        default: outport =
            lookupRoutingTable(route.vnet, route.net_dest);
            result[1] = outport;
            result[0] = -1;
            break;
    }

    assert(outport != -1);
    return result;
}

// XY routing implemented using port directions
// Only for reference purpose in a Mesh
// By default Garnet uses the routing table
int
RoutingUnit::outportComputeXY(RouteInfo route,
                              int inport,
                              PortDirection inport_dirn)
{
    PortDirection outport_dirn = "Unknown";

    [[maybe_unused]] int num_rows = m_router->get_net_ptr()->getNumRows();
    int num_cols = m_router->get_net_ptr()->getNumCols();
    assert(num_rows > 0 && num_cols > 0);

    int my_id = m_router->get_id();
    int my_x = my_id % num_cols;
    int my_y = my_id / num_cols;

    int dest_id = route.dest_router;
    int dest_x = dest_id % num_cols;
    int dest_y = dest_id / num_cols;

    int x_hops = abs(dest_x - my_x);
    int y_hops = abs(dest_y - my_y);

    bool x_dirn = (dest_x >= my_x);
    bool y_dirn = (dest_y >= my_y);

    // already checked that in outportCompute() function
    assert(!(x_hops == 0 && y_hops == 0));

    if (x_hops > 0) {
        if (x_dirn) {
            assert(inport_dirn == "Local" || inport_dirn == "West");
            outport_dirn = "East";
        } else {
            assert(inport_dirn == "Local" || inport_dirn == "East");
            outport_dirn = "West";
        }
    } else if (y_hops > 0) {
        if (y_dirn) {
            // "Local" or "South" or "West" or "East"
            assert(inport_dirn != "North");
            outport_dirn = "North";
        } else {
            // "Local" or "North" or "West" or "East"
            assert(inport_dirn != "South");
            outport_dirn = "South";
        }
    } else {
        // x_hops == 0 and y_hops == 0
        // this is not possible
        // already checked that in outportCompute() function
        panic("x_hops == y_hops == 0");
    }

    return m_outports_dirn2idx[outport_dirn];
}

// Template for implementing custom routing algorithm
// using port directions. (Example adaptive)
int
RoutingUnit::outportComputeCustom(RouteInfo route,
                                 int inport,
                                 PortDirection inport_dirn)
{
    // panic("%s placeholder executed", __FUNCTION__);
    PortDirection outport_dirn = "Unknown";

    int num_rows = m_router->get_net_ptr()->getNumRows();
    int num_cols = m_router->get_net_ptr()->getNumCols();
    // int num_rout = m_router->get_net_ptr()->getNumRouters();
    assert(num_rows > 0 && num_cols > 0);

    int my_id = m_router->get_id();
    int my_x = my_id % num_cols;
    int my_y = (my_id % (num_cols * num_rows)) / num_cols;
    int my_z = my_id / (num_cols * num_rows);

    int dest_id = route.dest_router;
    int dest_x = dest_id % num_cols;
    int dest_y = (dest_id % (num_cols * num_rows)) / num_cols;
    int dest_z = dest_id / (num_cols * num_rows);

    int x_hops = abs(dest_x - my_x);
    int y_hops = abs(dest_y - my_y);
    int z_hops = abs(dest_z - my_z);
    // int hop_array[3] = {x_hops, y_hops, z_hops};
    // std::cout<<hop_array[0]<<" "<<hop_array[1]<<" "<<hop_array[2]<<std::endl;

    bool x_dirn = (dest_x >= my_x);
    bool y_dirn = (dest_y >= my_y);
    bool z_dirn = (dest_z >= my_z);

    // int exa_array[3];
    // switch (int(x_dirn * 4 + y_dirn * 2 + z_dirn))
    // {
    // case 7:
    //     exa_array[0] = 2;
    //     exa_array[1] = 2;
    //     exa_array[2] = 3;
    //     break;
    // case 6:
    //     exa_array[0] = 3;
    //     exa_array[1] = 3;
    //     exa_array[2] = 3;
    //     break;
    // case 5:
    //     exa_array[0] = 0;
    //     exa_array[1] = 2;
    //     exa_array[2] = 2;
    //     break;
    // case 4:
    //     exa_array[0] = 1;
    //     exa_array[1] = 3;
    //     exa_array[2] = 2;
    //     break;
    // case 3:
    //     exa_array[0] = 2;
    //     exa_array[1] = 0;
    //     exa_array[2] = 1;
    //     break;
    // case 2:
    //     exa_array[0] = 3;
    //     exa_array[1] = 1;
    //     exa_array[2] = 1;
    //     break;
    // case 1:
    //     exa_array[0] = 0;
    //     exa_array[1] = 0;
    //     exa_array[2] = 0;
    //     break;
    // case 0:
    //     exa_array[0] = 1;
    //     exa_array[1] = 1;
    //     exa_array[2] = 0;
    //     break;
    // default:
    //     break;
    // }
    // std::default_random_engine e;
    // std::uniform_int_distribution<int> u(0,2); // 左闭右闭区间
    // e.seed(time(0));
    // int dir_ind;
    // dir_ind = u(e);
    // while (hop_array[dir_ind] == 0)
    // {
    //     dir_ind = u(e);
    // }
    // m_exa_vc = exa_array[dir_ind];
    // // std::cout<<m_exa_vc<<std::endl;
    // if (dir_ind == 0)
    // {
    //     if (x_dirn) {
    //         // assert(inport_dirn == "Local" || inport_dirn == "West");
    //         outport_dirn = "East";
    //     } else {
    //         // assert(inport_dirn == "Local" || inport_dirn == "East");
    //         outport_dirn = "West";
    //     }
    // } else if(dir_ind == 1)
    // {
    //     if (y_dirn) {
    //         outport_dirn = "North";
    //     } else {
    //         outport_dirn = "South";
    //     }
    // } else 
    // {
    //     if (z_dirn)
    //     {
    //         outport_dirn = "Down";
    //     } else{
    //         outport_dirn = "Up";
    //     }
    // }
    
    

    // already checked that in outportCompute() function
    // assert(!(x_hops == 0 && y_hops == 0 && z_hops == 0));

    if (x_hops > 0) {
        // m_exa_vc = exa_array[0];
        if (x_dirn) {
            assert(inport_dirn == "Local" || inport_dirn == "West");
            outport_dirn = "East";
        } else {
            assert(inport_dirn == "Local" || inport_dirn == "East");
            outport_dirn = "West";
        }
    } else if (y_hops > 0) {
        // m_exa_vc = exa_array[1];
        if (y_dirn) {
            // "Local" or "South" or "West" or "East"
            // assert(inport_dirn != "North");
            outport_dirn = "North";
        } else {
            // "Local" or "North" or "West" or "East"
            // assert(inport_dirn != "South");
            outport_dirn = "South";
        }
    } else if (z_hops > 0)
    {
        // m_exa_vc = exa_array[2];
        if (z_dirn)
        {
            outport_dirn = "Down";
        } else{
            outport_dirn = "Up";
        }
    } else {
        // x_hops == 0 and y_hops == 0
        // this is not possible
        // already checked that in outportCompute() function
        panic("x_hops == y_hops == z_hops == 0");
    }
    // std::cout<<m_exa_vc<<std::endl;

    return m_outports_dirn2idx[outport_dirn];
}
// int
// RoutingUnit::outportComputeCustom(RouteInfo route,
//                                  int inport,
//                                  PortDirection inport_dirn)
// {
//     // panic("%s placeholder executed", __FUNCTION__);
//     PortDirection outport_dirn = "Unknown";

//     // [[maybe_unused]] int num_rows = m_router->get_net_ptr()->getNumRows();
//     // std::cout<<num_rows<<std::endl;
//     // int num_cols = m_router->get_net_ptr()->getNumCols();
//     // std::cout<<num_cols<<std::endl;
//     int num_rout = m_router->get_net_ptr()->getNumRouters();
//     // std::cout<<num_rout<<std::endl;
//     // assert(num_rows > 0 && num_cols > 0);

//     int my_id = m_router->get_id();
//     int my_x = my_id % num_rout;
//     // int my_y = my_id / num_rout;

//     int dest_id = route.dest_router;
//     int dest_x = dest_id % num_rout;
//     // int dest_y = dest_id / num_rout;

//     int x_hops = dest_x - my_x;
//     // int y_hops = abs(dest_y - my_y);

//     bool x_dirn = ((0< x_hops < (num_rout / 2)) || (x_hops <= -(num_rout / 2)));
//     // bool y_dirn = (dest_y >= my_y);
    
//     // assert(x_hops > 0);

//     // std::cout<<inport_dirn<<std::endl;
//     if (x_dirn) {
//         // assert(inport_dirn == "Local" || inport_dirn == "West");
//         outport_dirn = "East";
//     } else {
//         // assert(inport_dirn == "Local" || inport_dirn == "East");
//         outport_dirn = "West";
//     }

//     return m_outports_dirn2idx[outport_dirn];
// }

} // namespace garnet
} // namespace ruby
} // namespace gem5
