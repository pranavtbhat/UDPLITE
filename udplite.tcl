#set constants
set error [lindex $argv 0]
set time [lindex $argv 1]
set pkt_size [lindex $argv 2]
set mode [lindex $argv 3]

#If constants weren't provided use defaults
if {$error == ""} {
	set error 0.0001
	set time 10.0
	set pkt_size 240
	set mode 0

	puts ""
	puts "Usage : ns udplite.tcl <error_rate> <time_of_simulation> <packet_size> <mode>"
	puts ""
	puts "Using default error of 0.0001"
	puts "Using default duration of simulation as 10.0s"
	puts "Using default packet size for audio : 240 bytes"
	puts "Using UDPLite mode: 0"
} 


#Create a simulator object
set ns [new Simulator]

#Open the NAM trace file

set tf [open out.tr w]
$ns trace-all $tf

#set nf [open udplite_out.nam w]
#$ns namtrace-all $nf

proc finish {} {
        global ns tf em udplite sink
        $ns flush-trace
        #close $nf
        close $tf
        #exec nam udplite_out.nam &
        puts ""

        set num_recv [$sink set pkts_recv_]
        set num_sent [$udplite set pkts_sent_]
        puts "Number of packets sent by source   : $num_sent"
        puts "Number of packets received at sink : $num_recv"

        set tp [expr "(double($num_recv)/$num_sent)*100"]
        puts "Observed throughput was            : $tp%"
        exit 0
}

set n0 [$ns node]
set n1 [$ns node]

$ns duplex-link $n0 $n1 2.5Mb 10ms DropTail
$ns duplex-link-op $n0 $n1 orient right

$ns duplex-link-op  $n0 $n1 queuePos 0.5 
$ns queue-limit $n0 $n1 1000

set udplite [new Agent/UDPLite]
$udplite set packetSize_ $pkt_size
$udplite set udp_mode_ $mode
$ns attach-agent $n0 $udplite

set sink [new Agent/UDPLite]
$sink set udp_mode_ $mode
$sink drop-target [new Agent/Null]
$ns attach-agent $n1 $sink

$ns connect $udplite $sink

set cbr [new Application/Traffic/CBR]
$cbr attach-agent $udplite

set em [new ErModel]
$em set rate_ $error
$em drop-target [new Agent/Null]

$ns link-lossmodel $em $n0 $n1

$ns at 0.0 "$cbr start"

$ns at [expr "$time-1.0"] "$cbr stop"
$ns at $time "finish"

$ns run


