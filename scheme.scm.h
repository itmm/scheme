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
"(define (map f ls . more)\n"
" (if (null? more)\n"
"     (let map1 ([ls ls])\n"
"       (if (null? ls)\n"
"           '()\n"
"\t   (cons (f (car ls))\n"
"\t         (map1 (cdr ls)))))\n"
"     (let map-more ([ls ls] [more more])\n"
"       (if (null? ls)\n"
"\t   '()\n"
"\t   (cons\n"
"\t     (apply f (car ls) (map car more))\n"
"\t     (map-more (cdr ls) (map cdr more)))))))\n"
