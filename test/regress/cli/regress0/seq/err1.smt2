; COMMAND-LINE: --seq-array=lazy
; EXPECT: unsat
(set-logic ALL)
(declare-sort E 0)
(declare-fun a () (Seq E))
(declare-fun a_ () (Seq E))
(declare-fun _3 () (Seq E))
(declare-fun _4 () (Seq E))
(declare-fun _39 () E)
(declare-fun e () E)
(assert (= (str.update _3 0 (seq.unit _39)) (str.update a 1 (seq.unit e))))
(assert (= a (str.update a 1 (seq.unit (seq.nth (str.update a 1 (seq.unit e)) 2)))))
(assert (= (str.update (str.update _3 0 (seq.unit e)) 0 (seq.unit _39)) (str.update _4 1 (seq.unit e))))
(assert (= e (seq.nth (str.update _4 1 (seq.unit e)) 2)))
(assert (distinct a (str.update _4 1 (seq.unit (seq.nth (str.update _3 0 (seq.unit e)) 1)))))
(check-sat)
