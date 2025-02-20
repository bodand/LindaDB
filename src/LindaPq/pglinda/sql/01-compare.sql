-- Adds comparison functions and operators to LV and LV_TYPE operand types.
-- Requires these types to be present (from types.sql).

--------------------------------------------------------------------------------
-- lv_lt: LV < LV, LV_TYPE < LV, LV < LV_TYPE, LV_TYPE < LV_TYPE
--------------------------------------------------------------------------------
DROP OPERATOR IF EXISTS < (LV, LV);
DROP OPERATOR IF EXISTS < (LV, LV_TYPE);
DROP OPERATOR IF EXISTS < (LV_TYPE, LV);
DROP OPERATOR IF EXISTS < (LV_TYPE, LV_TYPE);

DROP FUNCTION IF EXISTS lv_lt(LV, LV);
DROP FUNCTION IF EXISTS lv_lt(LV, LV_TYPE);
DROP FUNCTION IF EXISTS lv_lt(LV_TYPE, LV);
DROP FUNCTION IF EXISTS lv_lt(LV_TYPE, LV_TYPE);

CREATE FUNCTION lv_lt(LV, LV)
    RETURNS BOOLEAN
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so',
'linda_value_lt';

CREATE FUNCTION lv_lt(LV, LV_TYPE)
    RETURNS BOOLEAN
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so',
'linda_value_type_lt';

CREATE FUNCTION lv_lt(LV_TYPE, LV)
    RETURNS BOOLEAN
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so',
'linda_type_value_lt';

CREATE FUNCTION lv_lt(LV_TYPE, LV_TYPE)
    RETURNS BOOLEAN
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so',
'linda_type_lt';

CREATE OPERATOR < (
    FUNCTION = lv_lt,
    LEFTARG = LV,
    RIGHTARG = LV,
    COMMUTATOR = >,
    NEGATOR = >=,
    RESTRICT = scalarltsel,
    JOIN = scalarltjoinsel
    );

CREATE OPERATOR < (
    FUNCTION = lv_lt,
    LEFTARG = LV,
    RIGHTARG = LV_TYPE,
    COMMUTATOR = >,
    NEGATOR = >=,
    RESTRICT = scalarltsel,
    JOIN = scalarltjoinsel
    );

CREATE OPERATOR < (
    FUNCTION = lv_lt,
    LEFTARG = LV_TYPE,
    RIGHTARG = LV,
    COMMUTATOR = >,
    NEGATOR = >=,
    RESTRICT = scalarltsel,
    JOIN = scalarltjoinsel
    );

CREATE OPERATOR < (
    FUNCTION = lv_lt,
    LEFTARG = LV_TYPE,
    RIGHTARG = LV_TYPE,
    COMMUTATOR = >,
    NEGATOR = >=,
    RESTRICT = scalarltsel,
    JOIN = scalarltjoinsel
    );

--------------------------------------------------------------------------------
-- lv_le: LV <= LV, LV_TYPE <= LV, LV <= LV_TYPE, LV_TYPE <= LV_TYPE
--------------------------------------------------------------------------------
DROP OPERATOR IF EXISTS <= (LV, LV);
DROP OPERATOR IF EXISTS <= (LV, LV_TYPE);
DROP OPERATOR IF EXISTS <= (LV_TYPE, LV);
DROP OPERATOR IF EXISTS <= (LV_TYPE, LV_TYPE);

DROP FUNCTION IF EXISTS lv_le(LV, LV);
DROP FUNCTION IF EXISTS lv_le(LV, LV_TYPE);
DROP FUNCTION IF EXISTS lv_le(LV_TYPE, LV);
DROP FUNCTION IF EXISTS lv_le(LV_TYPE, LV_TYPE);

CREATE FUNCTION lv_le(LV, LV)
    RETURNS BOOLEAN
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so',
'linda_value_le';

CREATE FUNCTION lv_le(LV, LV_TYPE)
    RETURNS BOOLEAN
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so',
'linda_value_type_le';

CREATE FUNCTION lv_le(LV_TYPE, LV)
    RETURNS BOOLEAN
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so',
'linda_type_value_le';

CREATE FUNCTION lv_le(LV_TYPE, LV_TYPE)
    RETURNS BOOLEAN
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so',
'linda_type_le';

CREATE OPERATOR <= (
    FUNCTION = lv_le,
    LEFTARG = LV,
    RIGHTARG = LV,
    COMMUTATOR = >=,
    NEGATOR = >,
    RESTRICT = scalarlesel,
    JOIN = scalarlejoinsel
    );

CREATE OPERATOR <= (
    FUNCTION = lv_le,
    LEFTARG = LV,
    RIGHTARG = LV_TYPE,
    COMMUTATOR = >=,
    NEGATOR = >,
    RESTRICT = scalarlesel,
    JOIN = scalarlejoinsel
    );

CREATE OPERATOR <= (
    FUNCTION = lv_le,
    LEFTARG = LV_TYPE,
    RIGHTARG = LV,
    COMMUTATOR = >=,
    NEGATOR = >,
    RESTRICT = scalarlesel,
    JOIN = scalarlejoinsel
    );

CREATE OPERATOR <= (
    FUNCTION = lv_le,
    LEFTARG = LV_TYPE,
    RIGHTARG = LV_TYPE,
    COMMUTATOR = >=,
    NEGATOR = >,
    RESTRICT = scalarlesel,
    JOIN = scalarlejoinsel
    );

--------------------------------------------------------------------------------
-- lv_gt: LV > LV, LV_TYPE > LV, LV > LV_TYPE, LV_TYPE > LV_TYPE
--------------------------------------------------------------------------------
DROP OPERATOR IF EXISTS > (LV, LV);
DROP OPERATOR IF EXISTS > (LV, LV_TYPE);
DROP OPERATOR IF EXISTS > (LV_TYPE, LV);
DROP OPERATOR IF EXISTS > (LV_TYPE, LV_TYPE);

DROP FUNCTION IF EXISTS lv_gt(LV, LV);
DROP FUNCTION IF EXISTS lv_gt(LV, LV_TYPE);
DROP FUNCTION IF EXISTS lv_gt(LV_TYPE, LV);
DROP FUNCTION IF EXISTS lv_gt(LV_TYPE, LV_TYPE);

CREATE FUNCTION lv_gt(LV, LV)
    RETURNS BOOLEAN
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so',
'linda_value_gt';

CREATE FUNCTION lv_gt(LV, LV_TYPE)
    RETURNS BOOLEAN
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so',
'linda_value_type_gt';

CREATE FUNCTION lv_gt(LV_TYPE, LV)
    RETURNS BOOLEAN
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so',
'linda_type_value_gt';

CREATE FUNCTION lv_gt(LV_TYPE, LV_TYPE)
    RETURNS BOOLEAN
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so',
'linda_type_gt';

CREATE OPERATOR > (
    FUNCTION = lv_gt,
    LEFTARG = LV,
    RIGHTARG = LV,
    COMMUTATOR = <,
    NEGATOR = <=,
    RESTRICT = scalargtsel,
    JOIN = scalargtjoinsel
    );

CREATE OPERATOR > (
    FUNCTION = lv_gt,
    LEFTARG = LV,
    RIGHTARG = LV_TYPE,
    COMMUTATOR = <,
    NEGATOR = <=,
    RESTRICT = scalargtsel,
    JOIN = scalargtjoinsel
    );

CREATE OPERATOR > (
    FUNCTION = lv_gt,
    LEFTARG = LV_TYPE,
    RIGHTARG = LV,
    COMMUTATOR = <,
    NEGATOR = <=,
    RESTRICT = scalargtsel,
    JOIN = scalargtjoinsel
    );

CREATE OPERATOR > (
    FUNCTION = lv_gt,
    LEFTARG = LV_TYPE,
    RIGHTARG = LV_TYPE,
    COMMUTATOR = <,
    NEGATOR = <=,
    RESTRICT = scalargtsel,
    JOIN = scalargtjoinsel
    );

--------------------------------------------------------------------------------
-- lv_ge: LV >= LV, LV_TYPE >= LV, LV >= LV_TYPE, LV_TYPE >= LV_TYPE
--------------------------------------------------------------------------------
DROP OPERATOR IF EXISTS >= (LV, LV);
DROP OPERATOR IF EXISTS >= (LV, LV_TYPE);
DROP OPERATOR IF EXISTS >= (LV_TYPE, LV);
DROP OPERATOR IF EXISTS >= (LV_TYPE, LV_TYPE);

DROP FUNCTION IF EXISTS lv_ge(LV, LV);
DROP FUNCTION IF EXISTS lv_ge(LV, LV_TYPE);
DROP FUNCTION IF EXISTS lv_ge(LV_TYPE, LV);
DROP FUNCTION IF EXISTS lv_ge(LV_TYPE, LV_TYPE);

CREATE FUNCTION lv_ge(LV, LV)
    RETURNS BOOLEAN
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so',
'linda_value_ge';

CREATE FUNCTION lv_ge(LV, LV_TYPE)
    RETURNS BOOLEAN
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so',
'linda_value_type_ge';

CREATE FUNCTION lv_ge(LV_TYPE, LV)
    RETURNS BOOLEAN
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so',
'linda_type_value_ge';

CREATE FUNCTION lv_ge(LV_TYPE, LV_TYPE)
    RETURNS BOOLEAN
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so',
'linda_type_ge';

CREATE OPERATOR >= (
    FUNCTION = lv_ge,
    LEFTARG = LV,
    RIGHTARG = LV,
    COMMUTATOR = <=,
    NEGATOR = <,
    RESTRICT = scalargesel,
    JOIN = scalargejoinsel
    );

CREATE OPERATOR >= (
    FUNCTION = lv_ge,
    LEFTARG = LV,
    RIGHTARG = LV_TYPE,
    COMMUTATOR = <=,
    NEGATOR = <,
    RESTRICT = scalargesel,
    JOIN = scalargejoinsel
    );

CREATE OPERATOR >= (
    FUNCTION = lv_ge,
    LEFTARG = LV_TYPE,
    RIGHTARG = LV,
    COMMUTATOR = <=,
    NEGATOR = <,
    RESTRICT = scalargesel,
    JOIN = scalargejoinsel
    );

CREATE OPERATOR >= (
    FUNCTION = lv_ge,
    LEFTARG = LV_TYPE,
    RIGHTARG = LV_TYPE,
    COMMUTATOR = <=,
    NEGATOR = <,
    RESTRICT = scalargesel,
    JOIN = scalargejoinsel
    );

--------------------------------------------------------------------------------
-- lv_eq: LV = LV, LV_TYPE = LV, LV = LV_TYPE, LV_TYPE = LV_TYPE
--------------------------------------------------------------------------------
DROP OPERATOR IF EXISTS = (LV, LV);
DROP OPERATOR IF EXISTS = (LV, LV_TYPE);
DROP OPERATOR IF EXISTS = (LV_TYPE, LV);
DROP OPERATOR IF EXISTS = (LV_TYPE, LV_TYPE);

DROP FUNCTION IF EXISTS lv_eq(LV, LV);
DROP FUNCTION IF EXISTS lv_eq(LV, LV_TYPE);
DROP FUNCTION IF EXISTS lv_eq(LV_TYPE, LV);
DROP FUNCTION IF EXISTS lv_eq(LV_TYPE, LV_TYPE);

CREATE FUNCTION lv_eq(LV, LV)
    RETURNS BOOLEAN
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so',
'linda_value_eq';

CREATE FUNCTION lv_eq(LV, LV_TYPE)
    RETURNS BOOLEAN
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so',
'linda_value_type_eq';

CREATE FUNCTION lv_eq(LV_TYPE, LV)
    RETURNS BOOLEAN
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so',
'linda_type_value_eq';

CREATE FUNCTION lv_eq(LV_TYPE, LV_TYPE)
    RETURNS BOOLEAN
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so',
'linda_type_eq';

CREATE OPERATOR = (
    FUNCTION = lv_eq,
    LEFTARG = LV,
    RIGHTARG = LV,
    COMMUTATOR = =,
    NEGATOR = <>,
    RESTRICT = eqsel,
    JOIN = eqjoinsel,
    MERGES
    );

CREATE OPERATOR = (
    FUNCTION = lv_eq,
    LEFTARG = LV_TYPE,
    RIGHTARG = LV,
    COMMUTATOR = =,
    NEGATOR = <>,
    RESTRICT = eqsel,
    JOIN = eqjoinsel,
    MERGES
    );

CREATE OPERATOR = (
    FUNCTION = lv_eq,
    LEFTARG = LV,
    RIGHTARG = LV_TYPE,
    COMMUTATOR = =,
    NEGATOR = <>,
    RESTRICT = eqsel,
    JOIN = eqjoinsel,
    MERGES
    );

CREATE OPERATOR = (
    FUNCTION = lv_eq,
    LEFTARG = LV_TYPE,
    RIGHTARG = LV_TYPE,
    COMMUTATOR = =,
    NEGATOR = <>,
    RESTRICT = eqsel,
    JOIN = eqjoinsel,
    MERGES
    );

--------------------------------------------------------------------------------
-- lv_ne: LV <> LV, LV_TYPE <> LV, LV <> LV_TYPE, LV_TYPE <> LV_TYPE
--------------------------------------------------------------------------------
DROP OPERATOR IF EXISTS <> (LV, LV);
DROP OPERATOR IF EXISTS <> (LV, LV_TYPE);
DROP OPERATOR IF EXISTS <> (LV_TYPE, LV);
DROP OPERATOR IF EXISTS <> (LV_TYPE, LV_TYPE);

DROP FUNCTION IF EXISTS lv_ne(LV, LV);
DROP FUNCTION IF EXISTS lv_ne(LV, LV_TYPE);
DROP FUNCTION IF EXISTS lv_ne(LV_TYPE, LV);
DROP FUNCTION IF EXISTS lv_ne(LV_TYPE, LV_TYPE);

CREATE FUNCTION lv_ne(LV, LV)
    RETURNS BOOLEAN
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so',
'linda_value_ne';

CREATE FUNCTION lv_ne(LV, LV_TYPE)
    RETURNS BOOLEAN
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so',
'linda_value_type_ne';

CREATE FUNCTION lv_ne(LV_TYPE, LV)
    RETURNS BOOLEAN
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so',
'linda_type_value_ne';

CREATE FUNCTION lv_ne(LV_TYPE, LV_TYPE)
    RETURNS BOOLEAN
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so',
'linda_type_ne';

CREATE OPERATOR <> (
    FUNCTION = lv_ne,
    LEFTARG = LV,
    RIGHTARG = LV,
    COMMUTATOR = <>,
    NEGATOR = =,
    RESTRICT = neqsel,
    JOIN = neqjoinsel
    );

CREATE OPERATOR <> (
    FUNCTION = lv_ne,
    LEFTARG = LV_TYPE,
    RIGHTARG = LV,
    COMMUTATOR = <>,
    NEGATOR = =,
    RESTRICT = neqsel,
    JOIN = neqjoinsel
    );

CREATE OPERATOR <> (
    FUNCTION = lv_ne,
    LEFTARG = LV,
    RIGHTARG = LV_TYPE,
    COMMUTATOR = <>,
    NEGATOR = =,
    RESTRICT = neqsel,
    JOIN = neqjoinsel
    );

CREATE OPERATOR <> (
    FUNCTION = lv_ne,
    LEFTARG = LV_TYPE,
    RIGHTARG = LV_TYPE,
    COMMUTATOR = <>,
    NEGATOR = =,
    RESTRICT = neqsel,
    JOIN = neqjoinsel
    );

--------------------------------------------------------------------------------
-- lv_cmp: LV <=> LV, LV_TYPE <=> LV_TYPE
--------------------------------------------------------------------------------
DROP FUNCTION IF EXISTS lv_cmp(LV, LV);
DROP FUNCTION IF EXISTS lv_cmp(LV_TYPE, LV_TYPE);

CREATE FUNCTION lv_cmp(LV, LV)
    RETURNS int4
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so',
'linda_value_cmp';

CREATE FUNCTION lv_cmp(LV_TYPE, LV_TYPE)
    RETURNS int4
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so',
'linda_type_cmp';
