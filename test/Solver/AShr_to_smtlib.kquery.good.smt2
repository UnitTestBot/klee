;SMTLIBv2 Query 0
(set-logic QF_AUFBV )
(declare-fun A0 () (Array (_ BitVec 32) (_ BitVec 8) ) )
(declare-fun A1 () (Array (_ BitVec 32) (_ BitVec 8) ) )
(assert (and  (not  (=  (_ bv0 8) (ite (bvuge (select  A1 (_ bv0 32) ) (_ bv8 8) ) (_ bv0 8) (bvashr (select  A0 (_ bv0 32) ) (select  A1 (_ bv0 32) ) ) ) ) ) (bvule  (_ bv8 8) (select  A1 (_ bv0 32) ) ) ) )
(check-sat)
(exit)
