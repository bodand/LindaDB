DROP TYPE IF EXISTS LV CASCADE;

CREATE TYPE LV;

CREATE FUNCTION str_to_lv(CSTRING)
    RETURNS LV
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so' ,
'linda_value_str_to_datum';

CREATE FUNCTION lv_to_str(LV)
    RETURNS CSTRING
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so' ,
'datum_to_linda_value_str';

CREATE FUNCTION bytes_to_lv(INTERNAL)
    RETURNS LV
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so' ,
'linda_value_bytes_to_datum';

CREATE FUNCTION lv_to_bytes(LV)
    RETURNS bytea
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so' ,
'datum_to_linda_value_bytes';

CREATE TYPE LV
(
    INTERNALLENGTH = VARIABLE,
    INPUT = str_to_lv,
    OUTPUT = lv_to_str,
    RECEIVE = bytes_to_lv,
    SEND = lv_to_bytes,
    STORAGE = MAIN
);

CREATE FUNCTION lv_type(LV)
    RETURNS int2
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so' ,
'linda_value_type';

CREATE FUNCTION lv_nicetype(LV)
    RETURNS CSTRING
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so' ,
'linda_value_nicetype';

CREATE FUNCTION lv_lt(LV, LV)
    RETURNS BOOLEAN
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so' ,
'linda_value_lt';

CREATE OPERATOR < (
    FUNCTION = lv_lt,
    LEFTARG = LV,
    RIGHTARG = LV,
    COMMUTATOR = >,
    NEGATOR = >=,
    RESTRICT = scalarltsel,
    JOIN = scalarltjoinsel
    );

CREATE FUNCTION lv_le(LV, LV)
    RETURNS BOOLEAN
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so' ,
'linda_value_le';

CREATE OPERATOR <= (
    FUNCTION = lv_le,
    LEFTARG = LV,
    RIGHTARG = LV,
    COMMUTATOR = >=,
    NEGATOR = >,
    RESTRICT = scalarlesel,
    JOIN = scalarlejoinsel
    );

CREATE FUNCTION lv_gt(LV, LV)
    RETURNS BOOLEAN
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so' ,
'linda_value_gt';

CREATE OPERATOR > (
    FUNCTION = lv_gt,
    LEFTARG = LV,
    RIGHTARG = LV,
    COMMUTATOR = <,
    NEGATOR = <=,
    RESTRICT = scalargtsel,
    JOIN = scalargtjoinsel
    );

CREATE FUNCTION lv_ge(LV, LV)
    RETURNS BOOLEAN
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so' ,
'linda_value_ge';

CREATE OPERATOR >= (
    FUNCTION = lv_ge,
    LEFTARG = LV,
    RIGHTARG = LV,
    COMMUTATOR = <=,
    NEGATOR = <,
    RESTRICT = scalargesel,
    JOIN = scalargejoinsel
    );

CREATE FUNCTION lv_eq(LV, LV)
    RETURNS BOOLEAN
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so' ,
'linda_value_eq';

CREATE OPERATOR = (
    FUNCTION = lv_eq,
    LEFTARG = LV,
    RIGHTARG = LV,
    COMMUTATOR = =,
    NEGATOR = <>,
    RESTRICT = eqsel,
    JOIN = eqjoinsel,
--     HASHES,
    MERGES
    );

CREATE FUNCTION lv_ne(LV, LV)
    RETURNS BOOLEAN
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so' ,
'linda_value_ne';

CREATE OPERATOR <> (
    FUNCTION = lv_ne,
    LEFTARG = LV,
    RIGHTARG = LV,
    COMMUTATOR = <>,
    NEGATOR = =,
    RESTRICT = neqsel,
    JOIN = neqjoinsel
    );

CREATE FUNCTION lv_cmp(LV, LV)
    RETURNS int4
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so' ,
'linda_value_cmp';

CREATE OR REPLACE FUNCTION lv_eq_image(OID)
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

-- HASH support currently broken
--
-- CREATE FUNCTION lv_hash32(LV)
--     RETURNS int4
--     LANGUAGE C
--     STRICT
--     IMMUTABLE
--     PARALLEL SAFE
-- AS
-- '/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so' ,
-- 'linda_value_hash_32';
--
-- CREATE FUNCTION lv_hash64_salt(LV, int8)
--     RETURNS int8
--     LANGUAGE C
--     STRICT
--     IMMUTABLE
--     PARALLEL SAFE
-- AS
-- '/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so' ,
-- 'linda_value_hash_64salt';
--
-- CREATE OPERATOR CLASS lv_hash_class
--     DEFAULT FOR TYPE LV USING hash AS
--     OPERATOR 1 =
--     , FUNCTION 1 lv_hash32(LV)
--     , FUNCTION 2 lv_hash64_salt(LV, int8)
-- ;
