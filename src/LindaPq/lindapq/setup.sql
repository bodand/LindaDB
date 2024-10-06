-- /var/run/lindadata is a mounted ramdisk
CREATE TABLESPACE lindadata
    LOCATION '/var/run/lindadata'
  WITH ( RANDOM_PAGE_COST = 1.0
      , EFFECTIVE_IO_CONCURRENCY = 500
      , maintenance_io_concurrency = 500
       );

CREATE UNLOGGED TABLE linda_data
(
    id     BIGSERIAL PRIMARY KEY,
    first  LV,
    second LV,
    third  LV,
    others LV[]
)
    WITH (vacuum_truncate = FALSE)
    TABLESPACE lindadata
;

DROP TRIGGER IF EXISTS TR_linda_data_update ON linda_data;
CREATE TRIGGER TR_linda_data_update
    AFTER INSERT
    ON linda_data
    FOR EACH ROW
EXECUTE FUNCTION lv_notify_trigger();

DROP PROCEDURE IF EXISTS insert_linda(LV[]);
CREATE PROCEDURE insert_linda(vals LV[])
    LANGUAGE plpgsql
AS
$sql$
DECLARE
    others LV[] := NULL;
BEGIN
    IF ARRAY_LENGTH(vals, 1) >= 4 THEN others := vals[4:]; END IF;

    INSERT
      INTO linda_data(first, second, third, others)
    VALUES (vals[1], vals[2], vals[3], others);
END;
$sql$;

DO
$sql$
    BEGIN
        CALL insert_linda('{}');
        CALL insert_linda('{1@2,2@2,3@3,4@alma}');
    END;
$sql$;
