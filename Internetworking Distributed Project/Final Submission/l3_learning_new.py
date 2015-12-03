# Copyright 2011 James McCauley
#
# This file is part of POX.
#
# POX is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# POX is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with POX.  If not, see <http://www.gnu.org/licenses/>.

"""
A stupid L3 switch

For each switch:
1) Keep a table that maps IP addresses to MAC addresses and switch ports.
   Stock this table using information from ARP and IP packets.
2) When you see an ARP query, try to answer it using information in the table
   from step 1.  If the info in the table is old, just flood the query.
3) Flood all other ARPs.
4) When you see an IP packet, if you know the destination port (because it's
   in the table from step 1), install a flow for it.
"""

from pox.core import core
import pox
log = core.getLogger()

import pox.lib.packet as pkt
from pox.lib.packet.ethernet import ethernet
from pox.lib.packet.ipv4 import ipv4
from pox.lib.packet.arp import arp
# change
from pox.lib.packet.tcp import tcp
from pox.lib.packet.udp import udp
# include as part of the betta branch
from pox.openflow.of_json import *

import pox.openflow.libopenflow_01 as of

from pox.lib.revent import *

import time
import socket
import struct
import thread
import select

# Timeout for flows
FLOW_IDLE_TIMEOUT = 10

# Timeout for ARP entries
ARP_TIMEOUT = 60 * 2

s = socket.socket()
s2 = socket.socket()
Filename = "flowdump.txt"
Outputfile = open(Filename, 'w')

class Entry (object):
  """
  Not strictly an ARP entry.
  We use the port to determine which port to forward traffic out of.
  We use the MAC to answer ARP replies.
  We use the timeout so that if an entry is older than ARP_TIMEOUT, we
   flood the ARP request rather than try to answer it ourselves.
  """
  def __init__ (self, port, mac):
    self.timeout = time.time() + ARP_TIMEOUT
    self.port = port
    self.mac = mac

  def __eq__ (self, other):
    if type(other) == tuple:
      return (self.port,self.mac)==other
    else:
      return (self.port,self.mac)==(other.port,other.mac)
  def __ne__ (self, other):
    return not self.__eq__(other)

  def isExpired (self):
    return time.time() > self.timeout


class l3_switch (EventMixin):
  def __init__ (self):
    # For each switch, we map IP addresses to Entries
    self.arpTable = {}

    self.listenTo(core)

  def _handle_GoingUpEvent (self, event):
    self.listenTo(core.openflow)
    log.debug("Up...")

  def _handle_PacketIn (self, event):
    dpid = event.connection.dpid
    inport = event.port
    packet = event.parsed
   
    if not packet.parsed:
      log.warning("%i %i ignoring unparsed packet", dpid, inport)
      return

    if dpid not in self.arpTable:
      # New switch -- create an empty table
      self.arpTable[dpid] = {}

    if packet.type == ethernet.LLDP_TYPE:
      # Ignore LLDP packets
      return

    if isinstance(packet.next, ipv4):
      log.debug("%i %i IP %s => %s", dpid,inport,str(packet.next.srcip),str(packet.next.dstip))
      if isinstance(packet.next.next, tcp):
        l4pack = packet.next.next
        #log.info("S-port is %s", str(l4pack.srcport))
        if (str(l4pack.srcport) == "21"):
          if (str(l4pack.dstport) == "21"):
            #log.info("Source and Dest port are 21. Blocking...")
            msg = of.ofp_flow_mod(command=of.OFPFC_ADD,
                                  idle_timeout=of.OFP_FLOW_PERMANENT,
                                  hard_timeout=of.OFP_FLOW_PERMANENT,
                                  action=of.ofp_action_output(port = 0),
                                  match=of.ofp_match.from_packet(packet, inport))
            event.connection.send(msg.pack())

        if (str(l4pack.dstport) == "31337"):
          msg = of.ofp_flow_mod(command=of.OFPFC_ADD,
                                idle_timeout=of.OFP_FLOW_PERMANENT,
                                hard_timeout=of.OFP_FLOW_PERMANENT,
                                buffer_id=event.ofp.buffer_id,
                                action=of.ofp_action_output(port = 0),
                                match=of.ofp_match.from_packet(packet, inport\
))
          event.connection.send(msg.pack())
 
        if (str(l4pack.dstport) == "27374"):
          msg = of.ofp_flow_mod(command=of.OFPFC_ADD,
                                idle_timeout=of.OFP_FLOW_PERMANENT,
                                hard_timeout=of.OFP_FLOW_PERMANENT,
                                buffer_id=event.ofp.buffer_id,
                                action=of.ofp_action_output(port = 0),
                                match=of.ofp_match.from_packet(packet, inport\
))
          event.connection.send(msg.pack())

        #log.debug("Got TCP packet")
		
      if isinstance(packet.next.next, udp):
        l4pack = packet.next.next
        #log.debug("Got UDP packet")

      # Learn or update port/MAC info
      if packet.next.srcip in self.arpTable[dpid]:
        if self.arpTable[dpid][packet.next.srcip] != (inport, packet.src):
          log.info("%i %i RE-learned %s", dpid,inport,str(packet.next.srcip))
      else:
        log.debug("%i %i learned %s", dpid,inport,str(packet.next.srcip))
      self.arpTable[dpid][packet.next.srcip] = Entry(inport, packet.src)

      # Try to forward
      dstaddr = packet.next.dstip
      if dstaddr in self.arpTable[dpid]:
        # We have info about what port to send it out on...

        prt = self.arpTable[dpid][dstaddr].port
        if prt == inport:
          log.warning("%i %i not sending packet for %s back out of the input port" % (
           dpid, inport, str(dstaddr)))
        else:
          log.debug("%i %i installing flow for %s => %s out port %i" % (dpid,
            inport, str(packet.next.srcip), str(dstaddr), prt))

          msg = of.ofp_flow_mod(command=of.OFPFC_ADD,
                                idle_timeout=FLOW_IDLE_TIMEOUT,
                                hard_timeout=of.OFP_FLOW_PERMANENT,
                                buffer_id=event.ofp.buffer_id,
                                action=of.ofp_action_output(port = prt),
                                match=of.ofp_match.from_packet(packet, inport))
          event.connection.send(msg.pack())

    elif isinstance(packet.next, arp):
      a = packet.next
      log.debug("%i %i ARP %s %s => %s", dpid, inport,
       {arp.REQUEST:"request",arp.REPLY:"reply"}.get(a.opcode,
       'op:%i' % (a.opcode,)), str(a.protosrc), str(a.protodst))

      if a.prototype == arp.PROTO_TYPE_IP:
        if a.hwtype == arp.HW_TYPE_ETHERNET:
          if a.protosrc != 0:

            # Learn or update port/MAC info
            if a.protosrc in self.arpTable[dpid]:
              if self.arpTable[dpid][a.protosrc] != (inport, packet.src):
                log.info("%i %i RE-learned %s", dpid,inport,str(a.protosrc))
            else:
              log.debug("%i %i learned %s", dpid,inport,str(a.protosrc))
            self.arpTable[dpid][a.protosrc] = Entry(inport, packet.src)

            if a.opcode == arp.REQUEST:
              # Maybe we can answer

              if a.protodst in self.arpTable[dpid]:
                # We have an answer...

                if not self.arpTable[dpid][a.protodst].isExpired():
                  # .. and it's relatively current, so we'll reply ourselves

                  r = arp()
                  r.hwtype = a.hwtype
                  r.prototype = a.prototype
                  r.hwlen = a.hwlen
                  r.protolen = a.protolen
                  r.opcode = arp.REPLY
                  r.hwdst = a.hwsrc
                  r.protodst = a.protosrc
                  r.protosrc = a.protodst
                  r.hwsrc = self.arpTable[dpid][a.protodst].mac
                  e = ethernet(type=packet.type, src=r.hwsrc, dst=a.hwsrc)
                  e.set_payload(r)
                  log.debug("%i %i answering ARP for %s" % (dpid, inport,
                   str(r.protosrc)))
                  msg = of.ofp_packet_out()
                  msg.data = e.pack()
                  msg.actions.append(of.ofp_action_output(port =
                                                          of.OFPP_IN_PORT))
                  msg.in_port = inport
                  event.connection.send(msg)
                  return

      # Didn't know how to answer or otherwise handle this ARP, so just flood it
      log.debug("%i %i flooding ARP %s %s => %s" % (dpid, inport,
       {arp.REQUEST:"request",arp.REPLY:"reply"}.get(a.opcode,
       'op:%i' % (a.opcode,)), str(a.protosrc), str(a.protodst)))

      msg = of.ofp_packet_out(in_port = inport, action = of.ofp_action_output(port = of.OFPP_FLOOD))
      if event.ofp.buffer_id is of.NO_BUFFER:
        # Try sending the (probably incomplete) raw data
        msg.data = event.data
      else:
        msg.buffer_id = event.ofp.buffer_id
      event.connection.send(msg.pack())

    return
	
log = core.getLogger()

# handler for timer function that sends the requests to all the
# switches connected to the controller.
def _timer_func ():
  for connection in core.openflow._connections.values():
    connection.send(of.ofp_stats_request(body=of.ofp_flow_stats_request()))
    connection.send(of.ofp_stats_request(body=of.ofp_port_stats_request()))
  log.info("Sent %i flow/port stats request(s)", len(core.openflow._connections))

# handler to display flow statistics received in JSON format
# structure of event.stats is defined by ofp_flow_stats()
def _handle_flowstats_received (event):
  stats = flow_stats_to_list(event.stats)
  #log.info("FlowStatsReceived from %s: %s",
  #          dpidToStr(event.connection.dpid), stats)

# Get number of bytes/packets in flows for web traffic only
  #web_bytes = 0
  #web_flows = 0
  #web_packet = 0
  #duration = 0
  for f in event.stats:
    #if f.match.tp_dst == 1515 or f.match.tp_src == 1515:
    #web_bytes += f.byte_count
    #web_packet += f.packet_count
    #web_flows += 1
	sendstr = str(f.match.nw_proto)+'#'+str(f.packet_count)+'#'+str(f.byte_count)+'#'+str(f.duration_sec)+"#"+str(f.match.tp_src)+"#"+str(f.match.tp_dst)+"#"+str(f.match.nw_src)+"#"+str(f.match.nw_dst)+"#" 
 	global s
	s.send(sendstr)
	global Outputfile
	Outputfile.write(sendstr+"\n")
	del sendstr
  s.send("END#")
#  ready=select.select([s2],[],[],3)
#  if ready[0]:
#    log.info("in recv")
#	remove = s2.recv(65536)
#	log.debug("%s",remove)
#  print "not in recv"
  
  
# handler to display port statistics received in JSON format
def _handle_portstats_received (event):
  stats = flow_stats_to_list(event.stats)
  log.debug("PortStatsReceived from %s: %s",
            dpidToStr(event.connection.dpid), stats)

			
  


def launch ():
  from pox.lib.recoco import Timer

  # attach handsers to listners
  core.openflow.addListenerByName("FlowStatsReceived",
    _handle_flowstats_received)
  core.openflow.addListenerByName("PortStatsReceived",
    _handle_portstats_received)

  # timer set to execute every five seconds
  Timer(5, _timer_func, recurring=True)
  global s
  s.setblocking(0)
  s = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
  s.connect("/users/sc558bo/Final/sock_file")
  
  #global s2
  #s2.setblocking(0)
  #s2 = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
  #s2.connect("/users/sc558bo/Final/sock_file2")

  log.info("Setting up switch")
  core.registerNew(l3_switch)

