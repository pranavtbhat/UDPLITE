#set constants
set error [lindex $argv 0]
set time [lindex $argv 1]
set pkt_size [lindex $argv 2]
#Create a simulator object
set ns [new Simulator]

#Open the NAM trace file

set tf [open out.tr w]
$ns trace-all $tf

set nf [open udplite_out.nam w]
$ns namtrace-all $nf

proc finish {} {
        global ns nf tf em udplite sink
        $ns flush-trace
        close $nf
        close $tf
        exec nam udplite_out.nam &

        puts "[$em set rate_] [$sink set pkts_recv_] [$udplite set pkts_sent_]"
        exit 0
}

set n0 [$ns node]
set n1 [$ns node]

$ns duplex-link $n0 $n1 2.5Mb 10ms DropTail
$ns duplex-link-op $n0 $n1 orient right

set udplite [new Agent/UDPLite]
$udplite set packetSize_ $pkt_size
$udplite set udp_mode_ 0
$ns attach-agent $n0 $udplite

set sink [new Agent/UDPLite]
$sink set udp_mode_ 0
$ns attach-agent $n1 $sink

$ns connect $udplite $sink

set cbr [new Application/Traffic/CBR]
$cbr set packetSize_ $pkt_size
$cbr set rate_ 1mb
$cbr attach-agent $udplite

set em [new ErModel]
$em set rate_ $error
$em drop-target [new Agent/Null]

$ns link-lossmodel $em $n0 $n1

$ns at 0.0 "$cbr start"

$ns at [expr "$time-1.0"] "$cbr stop"
$ns at $time "finish"

$ns run


