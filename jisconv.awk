! /^#/ {
    print "Unicode_to_JIS0208[" $3 "] = " $2 ";" >> "encoding-set-utoj.h";
    print "JIS0208_to_Unicode[" $2 "] = " $3 ";" >> "encoding-set-jtou.h";
}
