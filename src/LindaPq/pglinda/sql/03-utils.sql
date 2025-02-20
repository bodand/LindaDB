-- Sets up various utilities

DROP FUNCTION IF EXISTS lv_type(LV);
CREATE FUNCTION lv_type(LV)
    RETURNS int2
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so' ,
'linda_value_type';

DROP FUNCTION IF EXISTS lv_truetype(LV);
CREATE FUNCTION lv_truetype(LV)
    RETURNS LV_TYPE
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so' ,
'linda_value_truetype';

DROP FUNCTION IF EXISTS lv_nicetype(LV);
CREATE FUNCTION lv_nicetype(LV)
    RETURNS VARCHAR
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so' ,
'linda_value_nicetype';

DROP FUNCTION IF EXISTS lv_notify_trigger();
CREATE FUNCTION lv_notify_trigger()
    RETURNS TRIGGER
    LANGUAGE C
    STRICT
    IMMUTABLE
    PARALLEL SAFE
AS
'/home/bodand/src/LindaDB/_build-release-x64-clang/src/LindaPq/libpglinda.so' ,
'linda_notify_trigger';
