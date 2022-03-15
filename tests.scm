#!./scheme
'arithmetic

'+
(and (assert (= (+) 0))
 (assert (= (+ 2) 2))
 (assert (= (+ -2) -2))
 (assert (= (+ 2 3) 5))
 (assert (= (+ 2 -3) -1))
 (assert (= (+ -2 3) 1))
 (assert (= (+ -2 -3) -5))
 (assert (= (+ 1 2 3 4) 10))
 (assert (= (+ 123456123456 876543876544) 1000000000000))
 (assert (= (+ (/ 2)) (/ 2)))
 (assert (= (+ (/ 2) (/ 3)) (/ 5 6)))
 (assert (= (+ (/ 2) 3) (/ 7 2)))
 (assert (= (+ 0.25) 0.25))
 (assert (= (+ 0.25 0.5) 0.75))
 (assert (= (+ 0.25 (/ 1 2)) 0.75))
 (assert (= (+ 0.25 2) 2.25)))

'-
(and (assert (= (-) 0))
 (assert (= (- 2) -2))
 (assert (= (- -2) 2))
 (assert (= (- 2 3) -1))
 (assert (= (- 3 2) 1))
 (assert (= (- -2 3) -5))
 (assert (= (- 3 -2) 5))
 (assert (= (- 2 -3) 5))
 (assert (= (- -3 2) -5))
 (assert (= (- 10 1 2 3) 4))
 (assert (= (- 1000000000000 876543876544) 123456123456))
 (assert (= (- (/ 2) (/ 4)) (/ 4)))
 (assert (= (- (/ 2) 2) (/ -3 2))))

'*
(and (assert (= (*) 1))
 (assert (= (* 3) 3))
 (assert (= (* 2 3) 6))
 (assert (= (* -2 3) -6))
 (assert (= (* 2 -3) -6))
 (assert (= (* -2 -3) 6))
 (assert (= (* 2 3 4) 24))
 (assert (= (* 12345 10000100001) 123451234512345))
 (assert (= (* (/ 2) (/ 2)) (/ 4)))
 (assert (= (* (/ 2) 3) (/ 3 2)))
 (assert (= (* (/ 2) 2) 1))
 (assert (= (* 0.5 1.5) 0.75))
 (assert (= (* 0.5 (/ 3 2)) 0.75))
 (assert (= (* 0.5 3) 1.5)))

'/
(and (assert (= (/) 1))
 (assert (= (/ 2) (/ 1 2)))
 (assert (= (/ 6 3) 2))
 (assert (= (/ 123451234512345 12345) 10000100001))
 (assert (= (/ 10000 100) 100)))

'remainder
(assert (= (remainder 6 3) 0))

'and
(and (assert (and))
 (assert (= (and 1) 1))
 (assert (not (and false)))
 (assert (= (and 1 2 3) 3)))

'or
(and (assert (not (or)))
 (assert (= (or 1) 1))
 (assert (not (or false)))
 (assert (= (or 1 2 3) 1)))
