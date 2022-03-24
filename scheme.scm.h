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
"(define (accumulate start op lst)\n"
"  (if (null? lst)\n"
"\tstart\n"
"\t(accumulate (op start (car lst))\n"
"\t            op\n"
"\t\t    (cdr lst))))\n"
"(define (@mk-simple-numeric op init)\n"
"  (lambda args (accumulate init op args)))\n"
"(define + (@mk-simple-numeric @binary+ 0))\n"
"(define * (@mk-simple-numeric @binary* 1))\n"
"(define (@mk-special-numeric op init)\n"
"  (lambda args\n"
"    (cond ((null? args)\n"
"\t\t   init)\n"
"          ((null? (cdr args))\n"
"\t\t   (op init (car args)))\n"
"\t\t  (else \n"
"\t\t\t(accumulate (car args)\n"
"\t\t\t\t\t\top (cdr args))))))\n"
"(define - (@mk-special-numeric @binary- 0))\n"
"(define / (@mk-special-numeric @binary/ 1))\n"
"\n"
