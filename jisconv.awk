! /^#/ {
    print "Unicode_to_JISX0208[" $3 "] = " $2 ";" >> "encoding-set-utoj.h";
    print "JISX0208_to_Unicode[" $2 "] = " $3 ";" >> "encoding-set-jtou.h";
}
