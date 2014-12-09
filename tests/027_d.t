# script-id and stack-trace

defun bar (x) {
    baz;
    println [stack-trace];
    baz;
};

defun foo (x) {
    bar (x);
    println [stack-trace];
    bar (x);
    println [stack-trace];
};

defun baz() {
    println [stack-trace];
};

defun bazz() {
    if {true} then: {
	println [stack-trace];
	defun bazzz() {};
	println "'bazzz' id: " [sid bazzz];
    };
};
