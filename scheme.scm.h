"(define nil ())\n"
"(define (cadr l)\n"
"  (car (cdr l)))\n"
"(define (cddr l)\n"
"  (cdr (cdr l)))\n"
"(define (caddr l)\n"
"  (car (cddr l)))\n"
"(define (cdddr l)\n"
"  (cdr (cddr l)))\n"
"(define (list . l) l)\n"
"(define true #t)\n"
"(define false #f)\n"
"(define (> a b)\n"
"  (< b a))\n"
"(define (not a) (if a #f #t))\n"
"\n"
"(define + (lambda-case\n"
"\t   (() 0)\n"
"\t   ((a) a)\n"
"\t   ((a b) (@binary+ a b))\n"
"\t   (x (apply + (cons (@binary+ (car x) (cadr x)) (cddr x))))))\n"
"(define - (lambda-case\n"
"\t   (() 0)\n"
"\t   ((a) (@binary- 0 a))\n"
"\t   ((a b) (@binary- a b))\n"
"\t   (x (apply - (cons (@binary- (car x) (cadr x)) (cddr x))))))\n"
"(define * (lambda-case\n"
"\t   (() 1)\n"
"\t   ((a) a)\n"
"\t   ((a b) (@binary* a b))\n"
"\t   (x (apply * (cons (@binary* (car x) (cadr x)) (cddr x))))))\n"
"(define / (lambda-case\n"
"\t   (() 1)\n"
"\t   ((a) (@binary/ 1 a))\n"
"\t   ((a b) (@binary/ a b))\n"
"\t   (x (apply / (cons (@binary/ (car x) (cadr x)) (cddr x))))))\n"
"\n"
