-- Creates the operator classes for LV and LV_TYPE types. These are defined
-- for B-Tree functionality. Hashes are not supported at the moment.
DROP OPERATOR CLASS lv_btree_class USING btree;
DROP OPERATOR CLASS lv_type_btree_class USING btree;

DROP FUNCTION lv_eq_image(OID);

CREATE FUNCTION lv_eq_image(OID)
    RETURNS bool
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so' ,
'linda_value_cmp_equal_image';

CREATE OPERATOR CLASS lv_btree_class
    DEFAULT FOR TYPE LV USING btree AS
    OPERATOR 1 <,
    OPERATOR 2 <=,
    OPERATOR 3 =,
    OPERATOR 4 >=,
    OPERATOR 5 >,
    FUNCTION 1 lv_cmp(LV, LV),
    FUNCTION 4 lv_eq_image(OID);

CREATE OPERATOR CLASS lv_type_btree_class
    DEFAULT FOR TYPE LV_TYPE USING btree AS
    OPERATOR 1 <,
    OPERATOR 2 <=,
    OPERATOR 3 =,
    OPERATOR 4 >=,
    OPERATOR 5 >,
    FUNCTION 1 lv_cmp(LV_TYPE, LV_TYPE);

