#!/usr/bin/env php
<?php
function tarai($x, $y, $z) {
    if ($x <= $y) {
        return $y;
    }
    return tarai(tarai($x-1, $y, $z),
    	         tarai($y-1, $z, $x),
		 tarai($z-1, $x, $y));
}
echo tarai(12, 6, 0);
echo "\n";
?>
