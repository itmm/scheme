#!./scheme
; examples from "Structure and Interpretation of Computer Programs"
; defines are scoped into functions to avoid namespace pollution

(assert 486)
(assert (= (+ 137 349) 486))
(assert (= (- 1000 334) 666))
(assert (= (* 5 99) 495))
(assert (= (/ 10 2) 5))
(assert (= (+ 2.7 10) 12.7))
(assert (= (+ 21 35 12 7) 75))
(assert (= (* 25 4 12) 1200))
(assert (= (+ (* 3 5) (- 10 6)) 19))
(assert (= (+ (* 3 (+ (* 2 4) (+ 3 5))) (+ (- 10 7) 6)) 57))
(assert (= (+ (* 3
	  (+ (* 2 4)
		 (+ 3 5)))
   (+ (- 10 7) 
	  6)) 57))
((lambda () 
   (define size 2)
   size
   (assert (= (* 5 size) 10))
))
(define (== a b)
   (define (abs x) (if (< x 0) (- x) x))
   (< (abs (- a b) x) 0.0001))
((lambda ()
   (define pi 3.14159)
   (define radius 10)
   (assert (== (* pi (* radius radius)) 314.159))
   (define circumference (* 2 pi radius))
   (assert (== circumference 62.8318))
))
((lambda ()
   (define (square x) (* x x))
   (assert (= (square 21) 441))
   (assert (= (square (+ 2 5)) 49))
   (define (sum-of-squares x y)
	 (+ (square x) (square y)))
   (assert (= (sum-of-squares 3 4) 25))
   (define (f a)
	 (sum-of-squares (+ a 1) (* a 2)))
   (assert (= (f 5) 136))))
((lambda ()
   (define (abs x)
	 (cond ((< x 0) (- x))
		   (else x)))
   (assert (= (abs 5) 5))
   (assert (= (abs (- 3)) 3))))
((lambda ()
   (define (abs x)
	 (if (< x 0) (- x) x))
   (assert (= (abs 5) 5))
   (assert (= (abs (- 3)) 3))

   (define (sqrt-iter guess x)
	 (if (good-enough? guess x)
	   guess (sqrt-iter (improve guess x) x)))
   (define (improve guess x)
	 (average guess (/ x guess)))

   (define (average x y)
	 (/ (+ x y) 2.0))

   (define (good-enough? guess x)
	 (< (abs (- (square guess) x)) 0.0001))

   (define (square x) (* x x))
   (define (sqrt x) (sqrt-iter 1.0 x))
   (assert (== (sqrt 9) 3))
   (assert (== (sqrt (+ 100 37)) 11.7047))
   (assert (== (sqrt (+ (sqrt 2) (sqrt 3))) 1.77377))
   (assert (== (square (sqrt 1000)) 1000))))

((lambda ()
   (define (count-change amount)
	 (cc amount 5))
   (define (cc amount kinds-of-coins)
	 (cond ((= amount 0) 1)
		   ((or (< amount 0) (= kinds-of-coins 0)) 0)
		   (else (+ (cc amount
						(- kinds-of-coins 1))
					(cc (- amount
						   (first-denomination kinds-of-coins))
						kinds-of-coins)))))
   (define (first-denomination kinds-of-coins)
	 (cond ((= kinds-of-coins 1) 1)
		   ((= kinds-of-coins 2) 5)
		   ((= kinds-of-coins 3) 10)
		   ((= kinds-of-coins 4) 25)
		   ((= kinds-of-coins 5) 50)))
   (assert (= (count-change 100) 292))))
(garbage-collect)
((lambda ()
   (define (even? n) (= (remainder n 2) 0))
   (define (square n) (* n n))
   (define (fast-expt b n)
	 (cond ((= n 0) 1)
		   ((even? n) (square (fast-expt b (/ n 2))))
		   (else (* b (fast-expt b (- n 1))))))
   (assert (= (fast-expt 3 5) 243))))
((lambda ()
   (define (gcd a b)
	 (if (= b 0)
	   a
	   (gcd b (remainder a b))))
   (assert (= (gcd 206 40) 2))))
((lambda ()
   (define (smallest-divisor n)
	 (find-divisor n 2))
   (define (square n) (* n n))
   (define (find-divisor n test-divisor)
	 (cond ((> (square test-divisor) n) n)
		   ((divides? test-divisor n) test-divisor)
		   (else (find-divisor n (+ test-divisor 1)))))
   (define (divides? a b)
	 (= (remainder b a) 0))
   (define (prime? n)
	 (= n (smallest-divisor n)))
   (assert (not (prime? 33)))
   (assert (prime? 11))))
((lambda ()
   (define (even? n) (= (remainder n 2) 0))
   (define (square n) (* n n))
   (define (expmod base exp m)
     (cond ((= exp 0) 1)
           ((even? exp) (remainder (square (expmod base (/ exp 2) m)) m))
	   (else (remainder (* base (expmod base (- exp 1) m))
					 m))))
   (define random (let ((seed 1234) (a 1664525) (c 1013904223) (m (* 65536 65536)))
     (lambda (max) 
       (set! seed (remainder (+ (* a seed) c) m))
       (remainder seed max))))
   (define (fermat-test n)
     (define (try-it a) (= (expmod a n n) a))
     (try-it (+ 1 (random (- n 1)))))
   (define (fast-prime? n times)
     (cond ((= times 0) true)
           ((fermat-test n) (fast-prime? n (- times 1)))
	   (else false)))
   (assert (not (fast-prime? 33 5)))
   (assert (fast-prime? 11 5))))
