proc tarai {x y z} {
    if {$x <= $y} {return $y}
    tarai [tarai [expr $x - 1] $y $z] \
	  [tarai [expr $y - 1] $z $x] \
	  [tarai [expr $z - 1] $x $y]
}
puts [tarai 12 6 0]
