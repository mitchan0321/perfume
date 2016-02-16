sub tarai {
    my ($x, $y, $z) = @_;
    if ($x <= $y) {
	return $y;
    }
    return &tarai(&tarai($x-1,$y,$z),
		  &tarai($y-1,$z,$x),
		  &tarai($z-1,$x,$y));
}
print &tarai(12,6,0),"\n";

