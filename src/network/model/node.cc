/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006 Georgia Tech Research Corporation, INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: George F. Riley<riley@ece.gatech.edu>
 *          Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include <iostream>
#include "node.h"
#include "node-list.h"
#include "net-device.h"
#include "application.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/object-vector.h"
#include "ns3/uinteger.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/global-value.h"
#include "ns3/boolean.h"
#include "ns3/simulator.h"
// #include "ns3/ipv4-ospf-routing.h"
// #include "ns3/settings.h"
#include "ns3/qbb-net-device.h"

NS_LOG_COMPONENT_DEFINE ("Node");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (Node);

GlobalValue g_checksumEnabled  = GlobalValue ("ChecksumEnabled",
                                              "A global switch to enable all checksums for all protocols",
                                              BooleanValue (false),
                                              MakeBooleanChecker ());

TypeId 
Node::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Node")
    .SetParent<Object> ()
    .AddConstructor<Node> ()
    .AddAttribute ("DeviceList", "The list of devices associated to this Node.",
                   ObjectVectorValue (),
                   MakeObjectVectorAccessor (&Node::m_devices),
                   MakeObjectVectorChecker<NetDevice> ())
    .AddAttribute ("ApplicationList", "The list of applications associated to this Node.",
                   ObjectVectorValue (),
                   MakeObjectVectorAccessor (&Node::m_applications),
                   MakeObjectVectorChecker<Application> ())
    .AddAttribute ("Id", "The id (unique integer) of this Node.",
                   TypeId::ATTR_GET, // allow only getting it.
                   UintegerValue (0),
                   MakeUintegerAccessor (&Node::m_id),
                   MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}

Node::Node()
  : m_id (0),
    m_sid (0)
{
	Construct ();
}

Node::Node(uint32_t sid)
  : m_id (0),
    m_sid (sid)
{
	Construct ();
}

void
Node::Construct (void)
{
	m_node_type = 0;
	m_id = NodeList::Add (this);
}

Node::~Node ()
{
}

uint32_t
Node::GetId (void) const
{
  return m_id;
}

uint32_t
Node::GetSystemId (void) const
{
  return m_sid;
}

uint32_t
Node::AddDevice (Ptr<NetDevice> device)
{
  uint32_t index = m_devices.size ();
  m_devices.push_back (device);
  device->SetNode (this);
  device->SetIfIndex (index);
  device->SetReceiveCallback (MakeCallback (&Node::NonPromiscReceiveFromDevice, this));
  Simulator::ScheduleWithContext (GetId (), Seconds (0.0), 
                                  &NetDevice::Start, device);
  NotifyDeviceAdded (device);
  return index;
}
Ptr<NetDevice>
Node::GetDevice (uint32_t index) const
{
  NS_ASSERT_MSG (index < m_devices.size (), "Device index " << index <<
                 " is out of range (only have " << m_devices.size () << " devices).");
  return m_devices[index];
}
uint32_t 
Node::GetNDevices (void) const
{
  return m_devices.size ();
}

uint32_t 
Node::AddApplication (Ptr<Application> application)
{
  uint32_t index = m_applications.size ();
  m_applications.push_back (application);
  application->SetNode (this);
  Simulator::ScheduleWithContext (GetId (), Seconds (0.0), 
                                  &Application::Start, application);
  return index;
}
Ptr<Application> 
Node::GetApplication (uint32_t index) const
{
  NS_ASSERT_MSG (index < m_applications.size (), "Application index " << index <<
                 " is out of range (only have " << m_applications.size () << " applications).");
  return m_applications[index];
}
uint32_t 
Node::GetNApplications (void) const
{
  return m_applications.size ();
}

void 
Node::DoDispose ()
{
  m_deviceAdditionListeners.clear ();
  m_handlers.clear ();
  for (std::vector<Ptr<NetDevice> >::iterator i = m_devices.begin ();
       i != m_devices.end (); i++)
    {
      Ptr<NetDevice> device = *i;
      device->Dispose ();
      *i = 0;
    }
  m_devices.clear ();
  for (std::vector<Ptr<Application> >::iterator i = m_applications.begin ();
       i != m_applications.end (); i++)
    {
      Ptr<Application> application = *i;
      application->Dispose ();
      *i = 0;
    }
  m_applications.clear ();
  Object::DoDispose ();
}
void 
Node::DoStart (void)
{
  for (std::vector<Ptr<NetDevice> >::iterator i = m_devices.begin ();
       i != m_devices.end (); i++)
    {
      Ptr<NetDevice> device = *i;
      device->Start();
    }
  for (std::vector<Ptr<Application> >::iterator i = m_applications.begin ();
       i != m_applications.end (); i++)
    {
      Ptr<Application> application = *i;
      application->Start();
    }

  Object::DoStart();
}

void
Node::RegisterProtocolHandler (ProtocolHandler handler, 
                               uint16_t protocolType,
                               Ptr<NetDevice> device,
                               bool promiscuous)
{
  struct Node::ProtocolHandlerEntry entry;
  entry.handler = handler;
  entry.protocol = protocolType;
  entry.device = device;
  entry.promiscuous = promiscuous;

  // On demand enable promiscuous mode in netdevices
  if (promiscuous)
    {
      if (device == 0)
        {
          for (std::vector<Ptr<NetDevice> >::iterator i = m_devices.begin ();
               i != m_devices.end (); i++)
            {
              Ptr<NetDevice> dev = *i;
              dev->SetPromiscReceiveCallback (MakeCallback (&Node::PromiscReceiveFromDevice, this));
            }
        }
      else
        {
          device->SetPromiscReceiveCallback (MakeCallback (&Node::PromiscReceiveFromDevice, this));
        }
    }

  m_handlers.push_back (entry);
}

void
Node::UnregisterProtocolHandler (ProtocolHandler handler)
{
  for (ProtocolHandlerList::iterator i = m_handlers.begin ();
       i != m_handlers.end (); i++)
    {
      if (i->handler.IsEqual (handler))
        {
          m_handlers.erase (i);
          break;
        }
    }
}

bool
Node::ChecksumEnabled (void)
{
  BooleanValue val;
  g_checksumEnabled.GetValue (val);
  return val.Get ();
}

bool
Node::PromiscReceiveFromDevice (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                                const Address &from, const Address &to, NetDevice::PacketType packetType)
{
  NS_LOG_FUNCTION (this);
  return ReceiveFromDevice (device, packet, protocol, from, to, packetType, true);
}

bool
Node::NonPromiscReceiveFromDevice (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                                   const Address &from)
{
  NS_LOG_FUNCTION (this);
  return ReceiveFromDevice (device, packet, protocol, from, device->GetAddress (), NetDevice::PacketType (0), false);
}

bool
Node::ReceiveFromDevice (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                         const Address &from, const Address &to, NetDevice::PacketType packetType, bool promiscuous)
{
  NS_ASSERT_MSG (Simulator::GetContext () == GetId (), "Received packet with erroneous context ; " <<
                 "make sure the channels in use are correctly updating events context " <<
                 "when transfering events from one node to another.");
  NS_LOG_DEBUG ("Node " << GetId () << " ReceiveFromDevice:  dev "
                        << device->GetIfIndex () << " (type=" << device->GetInstanceTypeId ().GetName ()
                        << ") Packet UID " << packet->GetUid ());
  bool found = false;

  for (ProtocolHandlerList::iterator i = m_handlers.begin ();
       i != m_handlers.end (); i++)
    {
      if (i->device == 0 ||
          (i->device != 0 && i->device == device))
        {
          if (i->protocol == 0 || 
              i->protocol == protocol)
            {
              if (promiscuous == i->promiscuous)
                {
                  i->handler (device, packet, protocol, from, to, packetType);
                  found = true;
                }
            }
        }
    }
  return found;
}
void 
Node::RegisterDeviceAdditionListener (DeviceAdditionListener listener)
{
  m_deviceAdditionListeners.push_back (listener);
  // and, then, notify the new listener about all existing devices.
  for (std::vector<Ptr<NetDevice> >::const_iterator i = m_devices.begin ();
       i != m_devices.end (); ++i)
    {
      listener (*i);
    }
}
void 
Node::UnregisterDeviceAdditionListener (DeviceAdditionListener listener)
{
  for (DeviceAdditionListenerList::iterator i = m_deviceAdditionListeners.begin ();
       i != m_deviceAdditionListeners.end (); i++)
    {
      if ((*i).IsEqual (listener))
        {
          m_deviceAdditionListeners.erase (i);
          break;
         }
    }
}
 
void 
Node::NotifyDeviceAdded (Ptr<NetDevice> device)
{
  for (DeviceAdditionListenerList::iterator i = m_deviceAdditionListeners.begin ();
       i != m_deviceAdditionListeners.end (); i++)
    {
      (*i) (device);
    }  
}

uint32_t Node::GetNodeType() {
    return m_node_type;
}

bool Node::SwitchReceiveFromDevice(Ptr<NetDevice> device, Ptr<Packet> packet, CustomHeader &ch){
	NS_ASSERT_MSG(false, "Calling SwitchReceiveFromDevice() on a non-switch node or this function is not implemented");
  return false;
}

void Node::SwitchNotifyDequeue(uint32_t ifIndex, uint32_t qIndex, Ptr<Packet> p){
	NS_ASSERT_MSG(false, "Calling NotifyDequeue() on a non-switch node or this function is not implemented");
}

// void Node::CalculateRoute(uint32_t host) {
//     // queue for the BFS.
//     std::vector<uint32_t> q;
//     // Distance from the host to each node.
//     std::map<uint32_t, int> dis;

//     Ptr<Ipv4OSPFRouting> m_ospf = this->GetObject<Ipv4OSPFRouting>();

//     //init 
//     q.push_back(host);
//     dis[host] = 0;

//     //BFS
//     for (int i = 0; i < (int)q.size(); i++) {
//         uint32_t now = q[i];
//         int d = dis[now];
//         for(auto it = m_ospf->nbr2if[now].begin(); it != m_ospf->nbr2if[now].end(); it++) {
//             if(!it->second.up) {
//                 continue;
//             }
//             uint32_t next = it->first;
//             // If 'next' have not been visited.
//             if(dis.find(next) == dis.end()) {
//                 dis[next] = d + 1;
//                 if(Settings::hostList[next].type == 1) {
//                 // if()
//                     q.push_back(next);
//                 }
//             }
//             // if 'now' is on the shortest path from 'next' to 'host'.
//             if(dis[next] == d + 1) {
//                 m_ospf->nextHop[next][host].push_back(now);
//             }
//         }
//     }
// }


// void Node::SetRoutingEntries() {
//     Ptr<Ipv4OSPFRouting> m_ospf = this->GetObject<Ipv4OSPFRouting>();
//     for(auto it = m_ospf->nextHop.begin(); it != m_ospf->nextHop.end(); it++) {
//         uint32_t node = it->first;
//         if(node == m_id) {
//             auto &table = it->second;
//             for (auto j = table.begin(); j != table.end(); ++j) {
//                 // The destination node id.
//                 uint32_t dst = j->first;
//                 Ipv4Address dstAddr = Ipv4Address(Settings::hostList[dst].ip);
//                 vector<uint32_t> nexts = j->second;
//                 for (int k = 0; k < (int)nexts.size(); k++) {
//                     uint32_t next = nexts[k];
//                     uint32_t interface = m_ospf->nbr2if[node][next].idx;
//                     m_ospf->AddTableEntry(dstAddr, interface);
//                 } 
//             }
//         }
//     }
// }

// void Node::CheckNeighbors() {
//   Ptr<Ipv4OSPFRouting> m_ospf = this->GetObject<Ipv4OSPFRouting>();
//   int index = m_ospf->checkNeighbors();

//   if(index != -1) {
//       // send lsa
//       const std::map<uint32_t, uint32_t>& hostId2IpMap = m_ospf->getHostId2IpMap();
//       std::map<uint32_t, Interface> tmp = m_ospf->nbr2if[m_id];
//       for (int i = 0; i < m_devices.size(); i++) {
//           for(auto it = tmp.begin(); it != tmp.end(); it++) {
//               if(it->second.idx == i) {
//                   uint32_t ip = hostId2IpMap.at(it->first);
//                   Ptr<QbbNetDevice> qbbdev = DynamicCast<QbbNetDevice>(m_devices[i]);
//                   qbbdev->sendLSAMessage(ip, index);
//               }
//           }
//       }

//       for(auto it1 = m_ospf->nbr2if.begin(); it1 != m_ospf->nbr2if.end(); it1++) {
//           // clear all up flags
//           for(auto it2 = it1->second.begin(); it2 != it1->second.end(); it2++) {
//               it2->second.up = false;
//           }
//       }
//       // update nbr2if
//       std::map<uint32_t, std::vector<uint32_t>> lsas = m_ospf->getLSAs();
//       for(auto it = lsas.begin(); it != lsas.end(); it++) {
//           uint32_t nodeId = it->first;
//           std::vector<uint32_t> neighbors = it->second;
//           for(auto it2 = neighbors.begin(); it2 != neighbors.end(); it2++) {
//               m_ospf->nbr2if[nodeId][*it2].up = true;
//               m_ospf->nbr2if[*it2][nodeId].up = true;
//           }
//       }
//       //clear nexthop
//       m_ospf->nextHop.clear();
//       // calculate route
//       for(auto it = Settings::hostList.begin(); it != Settings::hostList.end(); it++) {
//           CalculateRoute(it->id);
//       }
//       m_ospf->ClearTableEntry();
//       // set routing entries
//       SetRoutingEntries();
//   }
// }

// void Node::sendLSAToNei(int index) {
//     Ptr<Ipv4OSPFRouting> m_ospf = this->GetObject<Ipv4OSPFRouting>();
//     const std::map<uint32_t, uint32_t> &hostId2IpMap = m_ospf->getHostId2IpMap();
//     std::map<uint32_t, Interface> tmp = m_ospf->nbr2if[m_id];
//     for (int i = 0; i < m_devices.size(); i++) {
//         for (auto it = tmp.begin(); it != tmp.end(); it++) {
//             if (it->second.idx == i) {
//                 uint32_t ip = hostId2IpMap.at(it->first);
//                 Ptr<QbbNetDevice> qbbdev = DynamicCast<QbbNetDevice>(m_devices[i]);
//                 qbbdev->sendLSAMessage(ip, index);
//             }
//         }
//     }
// }
} // namespace ns3
