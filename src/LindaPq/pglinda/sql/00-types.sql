-- This script creates the LV and LV_TYPE types in postgres.
-- LV supports string and binary serialization, LV_TYPE ONLY supports string

--------------------------------------------------------------------------------
-- LV : A type representing a field of a linda tuple, that is a linda value
--------------------------------------------------------------------------------
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

--------------------------------------------------------------------------------
-- LV_TYPE : A type representing a type of a field of a linda tuple, that is a
--           type of a linda value
--------------------------------------------------------------------------------
DROP TYPE IF EXISTS LV_TYPE CASCADE;
CREATE TYPE LV_TYPE;

DROP FUNCTION IF EXISTS str_to_lvtype(CSTRING);
CREATE FUNCTION str_to_lvtype(CSTRING)
    RETURNS LV_TYPE
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so' ,
'linda_type_str_to_datum';

DROP FUNCTION IF EXISTS lvtype_to_str(LV_TYPE);
CREATE OR REPLACE FUNCTION lvtype_to_str(LV_TYPE)
    RETURNS CSTRING
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so' ,
'datum_to_linda_type_str';

DROP FUNCTION IF EXISTS bytes_to_lvtype(INTERNAL);
CREATE FUNCTION bytes_to_lvtype(INTERNAL)
    RETURNS LV_TYPE
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so' ,
'linda_type_bytes_to_datum';

DROP FUNCTION IF EXISTS lvtype_to_bytes(LV_TYPE);
CREATE FUNCTION lvtype_to_bytes(LV_TYPE)
    RETURNS bytea
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so' ,
'datum_to_linda_type_bytes';

CREATE TYPE LV_TYPE
(
    INPUT = str_to_lvtype,
    OUTPUT = lvtype_to_str,
    RECEIVE = bytes_to_lvtype,
    SEND = lvtype_to_bytes,
    ALIGNMENT = char,
    INTERNALLENGTH = 1,
    PASSEDBYVALUE
);
