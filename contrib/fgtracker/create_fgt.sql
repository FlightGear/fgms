--
-- PostgreSQL database dump
-- Environament: ubuntu 12.04 with postgresql 8.4 installed.
--

SET statement_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = off;
SET check_function_bodies = false;
SET client_min_messages = warning;
SET escape_string_warning = off;

--
-- Name: fgtracker; Type: COMMENT; Schema: -; Owner: fgtracker
--

COMMENT ON DATABASE fgtracker IS 'FlightGear tracker database';


SET search_path = public, pg_catalog;

--
-- Name: gtrgm; Type: SHELL TYPE; Schema: public; Owner: fgtracker
--

CREATE TYPE gtrgm;


--
-- Name: gtrgm_in(cstring); Type: FUNCTION; Schema: public; Owner: fgtracker
--

CREATE FUNCTION gtrgm_in(cstring) RETURNS gtrgm
    LANGUAGE c STRICT
    AS '/usr/lib/postgresql/8.4/lib/pg_trgm.so', 'gtrgm_in';


ALTER FUNCTION public.gtrgm_in(cstring) OWNER TO fgtracker;

--
-- Name: gtrgm_out(gtrgm); Type: FUNCTION; Schema: public; Owner: fgtracker
--

CREATE FUNCTION gtrgm_out(gtrgm) RETURNS cstring
    LANGUAGE c STRICT
    AS '/usr/lib/postgresql/8.4/lib/pg_trgm.so', 'gtrgm_out';


ALTER FUNCTION public.gtrgm_out(gtrgm) OWNER TO fgtracker;

--
-- Name: gtrgm; Type: TYPE; Schema: public; Owner: fgtracker
--

CREATE TYPE gtrgm (
    INTERNALLENGTH = variable,
    INPUT = gtrgm_in,
    OUTPUT = gtrgm_out,
    ALIGNMENT = int4,
    STORAGE = plain
);


ALTER TYPE public.gtrgm OWNER TO fgtracker;

--
-- Name: gtrgm_compress(internal); Type: FUNCTION; Schema: public; Owner: fgtracker
--

CREATE FUNCTION gtrgm_compress(internal) RETURNS internal
    LANGUAGE c
    AS '/usr/lib/postgresql/8.4/lib/pg_trgm.so', 'gtrgm_compress';


ALTER FUNCTION public.gtrgm_compress(internal) OWNER TO fgtracker;

--
-- Name: gtrgm_consistent(gtrgm, internal, integer); Type: FUNCTION; Schema: public; Owner: fgtracker
--

CREATE FUNCTION gtrgm_consistent(gtrgm, internal, integer) RETURNS boolean
    LANGUAGE c
    AS '/usr/lib/postgresql/8.4/lib/pg_trgm.so', 'gtrgm_consistent';


ALTER FUNCTION public.gtrgm_consistent(gtrgm, internal, integer) OWNER TO fgtracker;

--
-- Name: gtrgm_decompress(internal); Type: FUNCTION; Schema: public; Owner: fgtracker
--

CREATE FUNCTION gtrgm_decompress(internal) RETURNS internal
    LANGUAGE c
    AS '/usr/lib/postgresql/8.4/lib/pg_trgm.so', 'gtrgm_decompress';


ALTER FUNCTION public.gtrgm_decompress(internal) OWNER TO fgtracker;

--
-- Name: gtrgm_penalty(internal, internal, internal); Type: FUNCTION; Schema: public; Owner: fgtracker
--

CREATE FUNCTION gtrgm_penalty(internal, internal, internal) RETURNS internal
    LANGUAGE c STRICT
    AS '/usr/lib/postgresql/8.4/lib/pg_trgm.so', 'gtrgm_penalty';


ALTER FUNCTION public.gtrgm_penalty(internal, internal, internal) OWNER TO fgtracker;

--
-- Name: gtrgm_picksplit(internal, internal); Type: FUNCTION; Schema: public; Owner: fgtracker
--

CREATE FUNCTION gtrgm_picksplit(internal, internal) RETURNS internal
    LANGUAGE c
    AS '/usr/lib/postgresql/8.4/lib/pg_trgm.so', 'gtrgm_picksplit';


ALTER FUNCTION public.gtrgm_picksplit(internal, internal) OWNER TO fgtracker;

--
-- Name: gtrgm_same(gtrgm, gtrgm, internal); Type: FUNCTION; Schema: public; Owner: fgtracker
--

CREATE FUNCTION gtrgm_same(gtrgm, gtrgm, internal) RETURNS internal
    LANGUAGE c
    AS '/usr/lib/postgresql/8.4/lib/pg_trgm.so', 'gtrgm_same';


ALTER FUNCTION public.gtrgm_same(gtrgm, gtrgm, internal) OWNER TO fgtracker;

--
-- Name: gtrgm_union(bytea, internal); Type: FUNCTION; Schema: public; Owner: fgtracker
--

CREATE FUNCTION gtrgm_union(bytea, internal) RETURNS integer[]
    LANGUAGE c
    AS '/usr/lib/postgresql/8.4/lib/pg_trgm.so', 'gtrgm_union';


ALTER FUNCTION public.gtrgm_union(bytea, internal) OWNER TO fgtracker;

--
-- Name: set_limit(real); Type: FUNCTION; Schema: public; Owner: fgtracker
--

CREATE FUNCTION set_limit(real) RETURNS real
    LANGUAGE c IMMUTABLE STRICT
    AS '/usr/lib/postgresql/8.4/lib/pg_trgm.so', 'set_limit';


ALTER FUNCTION public.set_limit(real) OWNER TO fgtracker;

--
-- Name: show_limit(); Type: FUNCTION; Schema: public; Owner: fgtracker
--

CREATE FUNCTION show_limit() RETURNS real
    LANGUAGE c IMMUTABLE STRICT
    AS '/usr/lib/postgresql/8.4/lib/pg_trgm.so', 'show_limit';


ALTER FUNCTION public.show_limit() OWNER TO fgtracker;

--
-- Name: show_trgm(text); Type: FUNCTION; Schema: public; Owner: fgtracker
--

CREATE FUNCTION show_trgm(text) RETURNS text[]
    LANGUAGE c IMMUTABLE STRICT
    AS '/usr/lib/postgresql/8.4/lib/pg_trgm.so', 'show_trgm';


ALTER FUNCTION public.show_trgm(text) OWNER TO fgtracker;

--
-- Name: similarity(text, text); Type: FUNCTION; Schema: public; Owner: fgtracker
--

CREATE FUNCTION similarity(text, text) RETURNS real
    LANGUAGE c IMMUTABLE STRICT
    AS '/usr/lib/postgresql/8.4/lib/pg_trgm.so', 'similarity';


ALTER FUNCTION public.similarity(text, text) OWNER TO fgtracker;

--
-- Name: similarity_op(text, text); Type: FUNCTION; Schema: public; Owner: fgtracker
--

CREATE FUNCTION similarity_op(text, text) RETURNS boolean
    LANGUAGE c IMMUTABLE STRICT
    AS '/usr/lib/postgresql/8.4/lib/pg_trgm.so', 'similarity_op';


ALTER FUNCTION public.similarity_op(text, text) OWNER TO fgtracker;

--
-- Name: %; Type: OPERATOR; Schema: public; Owner: postgres
--

CREATE OPERATOR % (
    PROCEDURE = similarity_op,
    LEFTARG = text,
    RIGHTARG = text,
    COMMUTATOR = %,
    RESTRICT = contsel,
    JOIN = contjoinsel
);


ALTER OPERATOR public.% (text, text) OWNER TO postgres;

--
-- Name: gist_trgm_ops; Type: OPERATOR CLASS; Schema: public; Owner: postgres
--

CREATE OPERATOR CLASS gist_trgm_ops
    FOR TYPE text USING gist AS
    STORAGE gtrgm ,
    OPERATOR 1 %(text,text) ,
    FUNCTION 1 gtrgm_consistent(gtrgm,internal,integer) ,
    FUNCTION 2 gtrgm_union(bytea,internal) ,
    FUNCTION 3 gtrgm_compress(internal) ,
    FUNCTION 4 gtrgm_decompress(internal) ,
    FUNCTION 5 gtrgm_penalty(internal,internal,internal) ,
    FUNCTION 6 gtrgm_picksplit(internal,internal) ,
    FUNCTION 7 gtrgm_same(gtrgm,gtrgm,internal);


ALTER OPERATOR CLASS public.gist_trgm_ops USING gist OWNER TO postgres;

SET default_tablespace = '';

SET default_with_oids = false;

--
-- Name: IPC; Type: TABLE; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE TABLE "IPC" (
    variable character(50) NOT NULL,
    "IntValue" integer,
    "FloatValue" numeric(16,4),
    "StringValue" character(255),
    "TimestampValue" timestamp with time zone,
    "IntervalValue" interval
);


ALTER TABLE public."IPC" OWNER TO fgtracker;

--
-- Name: TABLE "IPC"; Type: COMMENT; Schema: public; Owner: fgtracker
--

COMMENT ON TABLE "IPC" IS 'Inter-process communication';


--
-- Name: airports; Type: TABLE; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE TABLE airports (
    id integer NOT NULL,
    icao text,
    name text
);


ALTER TABLE public.airports OWNER TO fgtracker;

--
-- Name: airports_id_seq; Type: SEQUENCE; Schema: public; Owner: fgtracker
--

CREATE SEQUENCE airports_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.airports_id_seq OWNER TO fgtracker;

--
-- Name: airports_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: fgtracker
--

ALTER SEQUENCE airports_id_seq OWNED BY airports.id;


--
-- Name: bloat; Type: VIEW; Schema: public; Owner: postgres
--

CREATE VIEW bloat AS
    SELECT sml.schemaname, sml.tablename, (sml.reltuples)::bigint AS reltuples, (sml.relpages)::bigint AS relpages, sml.otta, round(CASE WHEN (sml.otta = (0)::double precision) THEN 0.0 ELSE ((sml.relpages)::numeric / (sml.otta)::numeric) END, 1) AS tbloat, (((sml.relpages)::bigint)::double precision - sml.otta) AS wastedpages, (sml.bs * ((((sml.relpages)::double precision - sml.otta))::bigint)::numeric) AS wastedbytes, pg_size_pretty((((sml.bs)::double precision * ((sml.relpages)::double precision - sml.otta)))::bigint) AS wastedsize, sml.iname, (sml.ituples)::bigint AS ituples, (sml.ipages)::bigint AS ipages, sml.iotta, round(CASE WHEN ((sml.iotta = (0)::double precision) OR (sml.ipages = 0)) THEN 0.0 ELSE ((sml.ipages)::numeric / (sml.iotta)::numeric) END, 1) AS ibloat, CASE WHEN ((sml.ipages)::double precision < sml.iotta) THEN (0)::double precision ELSE (((sml.ipages)::bigint)::double precision - sml.iotta) END AS wastedipages, CASE WHEN ((sml.ipages)::double precision < sml.iotta) THEN (0)::double precision ELSE ((sml.bs)::double precision * ((sml.ipages)::double precision - sml.iotta)) END AS wastedibytes, CASE WHEN ((sml.ipages)::double precision < sml.iotta) THEN pg_size_pretty((0)::bigint) ELSE pg_size_pretty((((sml.bs)::double precision * ((sml.ipages)::double precision - sml.iotta)))::bigint) END AS wastedisize FROM (SELECT rs.schemaname, rs.tablename, cc.reltuples, cc.relpages, rs.bs, ceil(((cc.reltuples * (((((rs.datahdr + (rs.ma)::numeric) - CASE WHEN ((rs.datahdr % (rs.ma)::numeric) = (0)::numeric) THEN (rs.ma)::numeric ELSE (rs.datahdr % (rs.ma)::numeric) END))::double precision + rs.nullhdr2) + (4)::double precision)) / ((rs.bs)::double precision - (20)::double precision))) AS otta, COALESCE(c2.relname, '?'::name) AS iname, COALESCE(c2.reltuples, (0)::real) AS ituples, COALESCE(c2.relpages, 0) AS ipages, COALESCE(ceil(((c2.reltuples * ((rs.datahdr - (12)::numeric))::double precision) / ((rs.bs)::double precision - (20)::double precision))), (0)::double precision) AS iotta FROM (((((SELECT foo.ma, foo.bs, foo.schemaname, foo.tablename, ((foo.datawidth + (((foo.hdr + foo.ma) - CASE WHEN ((foo.hdr % foo.ma) = 0) THEN foo.ma ELSE (foo.hdr % foo.ma) END))::double precision))::numeric AS datahdr, (foo.maxfracsum * (((foo.nullhdr + foo.ma) - CASE WHEN ((foo.nullhdr % (foo.ma)::bigint) = 0) THEN (foo.ma)::bigint ELSE (foo.nullhdr % (foo.ma)::bigint) END))::double precision) AS nullhdr2 FROM (SELECT s.schemaname, s.tablename, constants.hdr, constants.ma, constants.bs, sum((((1)::double precision - s.null_frac) * (s.avg_width)::double precision)) AS datawidth, max(s.null_frac) AS maxfracsum, (constants.hdr + (SELECT (1 + (count(*) / 8)) FROM pg_stats s2 WHERE (((s2.null_frac <> (0)::double precision) AND (s2.schemaname = s.schemaname)) AND (s2.tablename = s.tablename)))) AS nullhdr FROM pg_stats s, (SELECT (SELECT (current_setting('block_size'::text))::numeric AS current_setting) AS bs, CASE WHEN ("substring"(foo.v, 12, 3) = ANY (ARRAY['8.0'::text, '8.1'::text, '8.2'::text])) THEN 27 ELSE 23 END AS hdr, CASE WHEN (foo.v ~ 'mingw32'::text) THEN 8 ELSE 4 END AS ma FROM (SELECT version() AS v) foo) constants GROUP BY s.schemaname, s.tablename, constants.hdr, constants.ma, constants.bs) foo) rs JOIN pg_class cc ON ((cc.relname = rs.tablename))) JOIN pg_namespace nn ON (((cc.relnamespace = nn.oid) AND (nn.nspname = rs.schemaname)))) LEFT JOIN pg_index i ON ((i.indrelid = cc.oid))) LEFT JOIN pg_class c2 ON ((c2.oid = i.indexrelid)))) sml WHERE ((((sml.relpages)::double precision - sml.otta) > (0)::double precision) OR (((sml.ipages)::double precision - sml.iotta) > (10)::double precision)) ORDER BY (sml.bs * ((((sml.relpages)::double precision - sml.otta))::bigint)::numeric) DESC, CASE WHEN ((sml.ipages)::double precision < sml.iotta) THEN (0)::double precision ELSE ((sml.bs)::double precision * ((sml.ipages)::double precision - sml.iotta)) END DESC;


ALTER TABLE public.bloat OWNER TO postgres;

--
-- Name: cache_time; Type: TABLE; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE TABLE cache_time (
    tablename character varying(100) NOT NULL,
    cachetime timestamp with time zone DEFAULT now()
);


ALTER TABLE public.cache_time OWNER TO fgtracker;

--
-- Name: cache_top100_alltime; Type: TABLE; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE TABLE cache_top100_alltime (
    callsign text,
    flighttime interval,
    rank integer,
    lastweek interval,
    last30days interval
);


ALTER TABLE public.cache_top100_alltime OWNER TO fgtracker;

--
-- Name: fgsync_log; Type: TABLE; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE TABLE fgsync_log (
    "timestamp" timestamp with time zone DEFAULT now(),
    fgsync_current_update_freq integer,
    fgsync_no_of_pilots_tracking integer,
    fgsync_no_of_new_waypoints integer,
    fgsync_no_of_new_flights integer,
    system_load numeric(5,2),
    fgsync_current_comparetick integer
);


ALTER TABLE public.fgsync_log OWNER TO fgtracker;

--
-- Name: fixes; Type: TABLE; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE TABLE fixes (
    id integer NOT NULL,
    latitude double precision,
    longitude double precision,
    name text NOT NULL
);


ALTER TABLE public.fixes OWNER TO fgtracker;

--
-- Name: fixes_id_seq; Type: SEQUENCE; Schema: public; Owner: fgtracker
--

CREATE SEQUENCE fixes_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.fixes_id_seq OWNER TO fgtracker;

--
-- Name: fixes_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: fgtracker
--

ALTER SEQUENCE fixes_id_seq OWNED BY fixes.id;


--
-- Name: flight_plans; Type: TABLE; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE TABLE flight_plans (
    id integer,
    seq integer,
    fix_name text
);


ALTER TABLE public.flight_plans OWNER TO fgtracker;

--
-- Name: flights; Type: TABLE; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE TABLE flights (
    id integer NOT NULL,
    callsign text,
    status text,
    model text,
    start_time timestamp with time zone,
    end_time timestamp with time zone,
    distance double precision,
    max_altimeter double precision,
    max_speed double precision
);


ALTER TABLE public.flights OWNER TO fgtracker;

--
-- Name: flights_archive; Type: TABLE; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE TABLE flights_archive (
    id integer NOT NULL,
    callsign text,
    status text,
    model text,
    start_time timestamp with time zone,
    end_time timestamp with time zone,
    distance double precision,
    max_altimeter double precision,
    max_speed double precision,
    wpts integer
);


ALTER TABLE public.flights_archive OWNER TO fgtracker;

--
-- Name: flights_all; Type: VIEW; Schema: public; Owner: fgtracker
--

CREATE VIEW flights_all AS
    SELECT flights.id, flights.callsign, flights.status, flights.model, flights.start_time, flights.end_time, flights.distance, flights.max_altimeter, flights.max_speed FROM flights UNION ALL SELECT flights_archive.id, flights_archive.callsign, flights_archive.status, flights_archive.model, flights_archive.start_time, flights_archive.end_time, flights_archive.distance, flights_archive.max_altimeter, flights_archive.max_speed FROM flights_archive;


ALTER TABLE public.flights_all OWNER TO fgtracker;

--
-- Name: flights_id_seq; Type: SEQUENCE; Schema: public; Owner: fgtracker
--

CREATE SEQUENCE flights_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.flights_id_seq OWNER TO fgtracker;

--
-- Name: flights_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: fgtracker
--

ALTER SEQUENCE flights_id_seq OWNED BY flights.id;


--
-- Name: flights_old; Type: TABLE; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE TABLE flights_old (
    id integer,
    callsign text,
    status text,
    model text,
    start_time timestamp with time zone,
    end_time timestamp with time zone,
    distance double precision,
    max_altimeter double precision,
    max_speed double precision
);


ALTER TABLE public.flights_old OWNER TO fgtracker;

--
-- Name: log; Type: TABLE; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE TABLE log (
    username text,
    "table" text,
    action text,
    "when" timestamp with time zone,
    callsign text
);


ALTER TABLE public.log OWNER TO fgtracker;

--
-- Name: models; Type: TABLE; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE TABLE models (
    fg_string text NOT NULL,
    human_string text
);


ALTER TABLE public.models OWNER TO fgtracker;

--
-- Name: navaid_types; Type: TABLE; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE TABLE navaid_types (
    id integer NOT NULL,
    label text
);


ALTER TABLE public.navaid_types OWNER TO fgtracker;

--
-- Name: navaids; Type: TABLE; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE TABLE navaids (
    id integer NOT NULL,
    type integer,
    latitude double precision,
    longitude double precision,
    range integer,
    freq double precision,
    ilt text,
    name text
);


ALTER TABLE public.navaids OWNER TO fgtracker;

--
-- Name: navaids_id_seq; Type: SEQUENCE; Schema: public; Owner: fgtracker
--

CREATE SEQUENCE navaids_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.navaids_id_seq OWNER TO fgtracker;

--
-- Name: navaids_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: fgtracker
--

ALTER SEQUENCE navaids_id_seq OWNED BY navaids.id;


--
-- Name: route_points; Type: TABLE; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE TABLE route_points (
    route_id integer,
    seq integer,
    navaid_id integer
);


ALTER TABLE public.route_points OWNER TO fgtracker;

--
-- Name: routes; Type: TABLE; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE TABLE routes (
    id integer NOT NULL,
    status text,
    dep_airport_id integer,
    dep_runway text,
    arr_airport_id integer,
    arr_runway text,
    plan_date timestamp without time zone,
    types text,
    mind real,
    maxd real
);


ALTER TABLE public.routes OWNER TO fgtracker;

--
-- Name: routes_id_seq; Type: SEQUENCE; Schema: public; Owner: fgtracker
--

CREATE SEQUENCE routes_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.routes_id_seq OWNER TO fgtracker;

--
-- Name: routes_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: fgtracker
--

ALTER SEQUENCE routes_id_seq OWNED BY routes.id;


--
-- Name: runways; Type: TABLE; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE TABLE runways (
    airport_id integer,
    runway text,
    latitude double precision,
    longitude double precision,
    llz_freq double precision,
    llz_ilt text,
    llz_name text
);


ALTER TABLE public.runways OWNER TO fgtracker;

--
-- Name: temp_cache_top100_alltime; Type: TABLE; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE TABLE temp_cache_top100_alltime (
    callsign text,
    flighttime interval,
    rank integer NOT NULL
);


ALTER TABLE public.temp_cache_top100_alltime OWNER TO fgtracker;

--
-- Name: temp_cache_top100_alltime_rank_seq; Type: SEQUENCE; Schema: public; Owner: fgtracker
--

CREATE SEQUENCE temp_cache_top100_alltime_rank_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.temp_cache_top100_alltime_rank_seq OWNER TO fgtracker;

--
-- Name: temp_cache_top100_alltime_rank_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: fgtracker
--

ALTER SEQUENCE temp_cache_top100_alltime_rank_seq OWNED BY temp_cache_top100_alltime.rank;


--
-- Name: temp_flight_id; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE temp_flight_id (
    flight_id integer
);


ALTER TABLE public.temp_flight_id OWNER TO postgres;

--
-- Name: temp_flights; Type: TABLE; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE TABLE temp_flights (
    id integer DEFAULT nextval('flights_id_seq'::regclass) NOT NULL,
    callsign text,
    status text,
    model text,
    start_time timestamp with time zone,
    end_time timestamp with time zone,
    distance double precision,
    max_altimeter double precision,
    max_speed double precision
);


ALTER TABLE public.temp_flights OWNER TO fgtracker;

--
-- Name: tracker_stats; Type: TABLE; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE TABLE tracker_stats (
    month text NOT NULL,
    count integer,
    modified timestamp without time zone
);


ALTER TABLE public.tracker_stats OWNER TO fgtracker;

--
-- Name: users; Type: TABLE; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE TABLE users (
    xuser text,
    db_callsign text
);


ALTER TABLE public.users OWNER TO fgtracker;

--
-- Name: waypoints; Type: TABLE; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE TABLE waypoints (
    id integer NOT NULL,
    flight_id integer,
    "time" timestamp with time zone,
    latitude double precision,
    longitude double precision,
    altitude double precision
);


ALTER TABLE public.waypoints OWNER TO fgtracker;

--
-- Name: waypoints_archive; Type: TABLE; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE TABLE waypoints_archive (
    id integer NOT NULL,
    flight_id integer,
    "time" timestamp with time zone,
    latitude double precision,
    longitude double precision,
    altitude double precision
);


ALTER TABLE public.waypoints_archive OWNER TO fgtracker;

--
-- Name: waypoints_all; Type: VIEW; Schema: public; Owner: fgtracker
--

CREATE VIEW waypoints_all AS
    SELECT waypoints.id, waypoints.flight_id, waypoints."time", waypoints.latitude, waypoints.longitude, waypoints.altitude FROM waypoints UNION ALL SELECT waypoints_archive.id, waypoints_archive.flight_id, waypoints_archive."time", waypoints_archive.latitude, waypoints_archive.longitude, waypoints_archive.altitude FROM waypoints_archive;


ALTER TABLE public.waypoints_all OWNER TO fgtracker;

--
-- Name: waypoints_id_seq; Type: SEQUENCE; Schema: public; Owner: fgtracker
--

CREATE SEQUENCE waypoints_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.waypoints_id_seq OWNER TO fgtracker;

--
-- Name: waypoints_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: fgtracker
--

ALTER SEQUENCE waypoints_id_seq OWNED BY waypoints.id;


--
-- Name: waypoints_last_month; Type: VIEW; Schema: public; Owner: fgtracker
--

CREATE VIEW waypoints_last_month AS
    SELECT count(*) AS count FROM waypoints WHERE ((date_part('year'::text, waypoints."time") = date_part('year'::text, (now() - '1 mon'::interval))) AND (date_part('month'::text, waypoints."time") = date_part('month'::text, (now() - '1 mon'::interval))));


ALTER TABLE public.waypoints_last_month OWNER TO fgtracker;

--
-- Name: VIEW waypoints_last_month; Type: COMMENT; Schema: public; Owner: fgtracker
--

COMMENT ON VIEW waypoints_last_month IS 'Count last months waypoints added to the database.';


--
-- Name: waypoints_old; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE waypoints_old (
    id integer,
    flight_id integer,
    "time" timestamp with time zone,
    latitude double precision,
    longitude double precision,
    altitude double precision
);


ALTER TABLE public.waypoints_old OWNER TO postgres;

--
-- Name: id; Type: DEFAULT; Schema: public; Owner: fgtracker
--

ALTER TABLE ONLY airports ALTER COLUMN id SET DEFAULT nextval('airports_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: fgtracker
--

ALTER TABLE ONLY fixes ALTER COLUMN id SET DEFAULT nextval('fixes_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: fgtracker
--

ALTER TABLE ONLY flights ALTER COLUMN id SET DEFAULT nextval('flights_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: fgtracker
--

ALTER TABLE ONLY navaids ALTER COLUMN id SET DEFAULT nextval('navaids_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: fgtracker
--

ALTER TABLE ONLY routes ALTER COLUMN id SET DEFAULT nextval('routes_id_seq'::regclass);


--
-- Name: rank; Type: DEFAULT; Schema: public; Owner: fgtracker
--

ALTER TABLE ONLY temp_cache_top100_alltime ALTER COLUMN rank SET DEFAULT nextval('temp_cache_top100_alltime_rank_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: fgtracker
--

ALTER TABLE ONLY waypoints ALTER COLUMN id SET DEFAULT nextval('waypoints_id_seq'::regclass);


--
-- Name: IPC_pkey; Type: CONSTRAINT; Schema: public; Owner: fgtracker; Tablespace: 
--

ALTER TABLE ONLY "IPC"
    ADD CONSTRAINT "IPC_pkey" PRIMARY KEY (variable);


--
-- Name: airports_pkey; Type: CONSTRAINT; Schema: public; Owner: fgtracker; Tablespace: 
--

ALTER TABLE ONLY airports
    ADD CONSTRAINT airports_pkey PRIMARY KEY (id);


--
-- Name: fix_name; Type: CONSTRAINT; Schema: public; Owner: fgtracker; Tablespace: 
--

ALTER TABLE ONLY fixes
    ADD CONSTRAINT fix_name PRIMARY KEY (name);


--
-- Name: flights_archive_pkey; Type: CONSTRAINT; Schema: public; Owner: fgtracker; Tablespace: 
--

ALTER TABLE ONLY flights_archive
    ADD CONSTRAINT flights_archive_pkey PRIMARY KEY (id);


--
-- Name: flights_pkey; Type: CONSTRAINT; Schema: public; Owner: fgtracker; Tablespace: 
--

ALTER TABLE ONLY flights
    ADD CONSTRAINT flights_pkey PRIMARY KEY (id);


--
-- Name: models_pkey; Type: CONSTRAINT; Schema: public; Owner: fgtracker; Tablespace: 
--

ALTER TABLE ONLY models
    ADD CONSTRAINT models_pkey PRIMARY KEY (fg_string);


--
-- Name: navaid_types_pkey; Type: CONSTRAINT; Schema: public; Owner: fgtracker; Tablespace: 
--

ALTER TABLE ONLY navaid_types
    ADD CONSTRAINT navaid_types_pkey PRIMARY KEY (id);


--
-- Name: temp_flights_pkey; Type: CONSTRAINT; Schema: public; Owner: fgtracker; Tablespace: 
--

ALTER TABLE ONLY temp_flights
    ADD CONSTRAINT temp_flights_pkey PRIMARY KEY (id);


--
-- Name: tracker_stats_pkey; Type: CONSTRAINT; Schema: public; Owner: fgtracker; Tablespace: 
--

ALTER TABLE ONLY tracker_stats
    ADD CONSTRAINT tracker_stats_pkey PRIMARY KEY (month);


--
-- Name: waypoints_archive_pkey; Type: CONSTRAINT; Schema: public; Owner: fgtracker; Tablespace: 
--

ALTER TABLE ONLY waypoints_archive
    ADD CONSTRAINT waypoints_archive_pkey PRIMARY KEY (id);


--
-- Name: waypoints_pkey; Type: CONSTRAINT; Schema: public; Owner: fgtracker; Tablespace: 
--

ALTER TABLE ONLY waypoints
    ADD CONSTRAINT waypoints_pkey PRIMARY KEY (id);


--
-- Name: airports_icao_idx; Type: INDEX; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE UNIQUE INDEX airports_icao_idx ON airports USING btree (icao);


--
-- Name: fixes-id-idx; Type: INDEX; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE UNIQUE INDEX "fixes-id-idx" ON fixes USING btree (id);


--
-- Name: flight_plans-id-idx; Type: INDEX; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE INDEX "flight_plans-id-idx" ON flight_plans USING btree (id);


--
-- Name: flight_plans-seq-idx; Type: INDEX; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE INDEX "flight_plans-seq-idx" ON flight_plans USING btree (seq);


--
-- Name: flights-times-idx; Type: INDEX; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE INDEX "flights-times-idx" ON flights USING btree (start_time, end_time);


--
-- Name: flights_archive-callsign-idx; Type: INDEX; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE INDEX "flights_archive-callsign-idx" ON flights_archive USING btree (callsign);


--
-- Name: flights_callsign; Type: INDEX; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE INDEX flights_callsign ON flights USING btree (callsign);


--
-- Name: flights_status; Type: INDEX; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE INDEX flights_status ON flights USING btree (status);


--
-- Name: navaids-id-idx; Type: INDEX; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE INDEX "navaids-id-idx" ON navaids USING btree (id);


--
-- Name: navaids-ilt-idx; Type: INDEX; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE INDEX "navaids-ilt-idx" ON navaids USING btree (ilt);


--
-- Name: navaids-lat_lon-idx; Type: INDEX; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE INDEX "navaids-lat_lon-idx" ON navaids USING btree (latitude, longitude);


--
-- Name: navaids_type_idx; Type: INDEX; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE INDEX navaids_type_idx ON navaids USING btree (type);


--
-- Name: route_points-route_id-idx; Type: INDEX; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE INDEX "route_points-route_id-idx" ON route_points USING btree (route_id);


--
-- Name: route_points-seq-idx; Type: INDEX; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE INDEX "route_points-seq-idx" ON route_points USING btree (seq);


--
-- Name: routes-arr-idx; Type: INDEX; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE INDEX "routes-arr-idx" ON routes USING btree (arr_airport_id, arr_runway);


--
-- Name: routes-dep-idx; Type: INDEX; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE INDEX "routes-dep-idx" ON routes USING btree (dep_airport_id, dep_runway);


--
-- Name: routes-id-idx; Type: INDEX; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE INDEX "routes-id-idx" ON routes USING btree (id);


--
-- Name: routes-limits-idx; Type: INDEX; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE INDEX "routes-limits-idx" ON routes USING btree (types, mind, maxd);


--
-- Name: routes-status-idx; Type: INDEX; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE INDEX "routes-status-idx" ON routes USING btree (status);


--
-- Name: runways-airport_id-idx; Type: INDEX; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE INDEX "runways-airport_id-idx" ON runways USING btree (airport_id);


--
-- Name: temp_flights_callsign_key; Type: INDEX; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE INDEX temp_flights_callsign_key ON temp_flights USING btree (callsign);


--
-- Name: temp_flights_start_time_key; Type: INDEX; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE INDEX temp_flights_start_time_key ON temp_flights USING btree (start_time, end_time);


--
-- Name: temp_flights_status_key; Type: INDEX; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE INDEX temp_flights_status_key ON temp_flights USING btree (status);


--
-- Name: waypoints-flight_id-idx; Type: INDEX; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE INDEX "waypoints-flight_id-idx" ON waypoints USING btree (flight_id);

ALTER TABLE waypoints CLUSTER ON "waypoints-flight_id-idx";


--
-- Name: waypoints-time-idx; Type: INDEX; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE INDEX "waypoints-time-idx" ON waypoints USING btree ("time");


--
-- Name: waypoints_archive-flight_id-idx; Type: INDEX; Schema: public; Owner: fgtracker; Tablespace: 
--

CREATE INDEX "waypoints_archive-flight_id-idx" ON waypoints_archive USING btree (flight_id);


--
-- Name: del_wp; Type: RULE; Schema: public; Owner: fgtracker
--

CREATE RULE del_wp AS ON DELETE TO flights WHERE (old.status = 'CLOSED'::text) DO DELETE FROM waypoints WHERE (waypoints.flight_id = old.id);


--
-- Name: public; Type: ACL; Schema: -; Owner: fgtracker
--

REVOKE ALL ON SCHEMA public FROM PUBLIC;
REVOKE ALL ON SCHEMA public FROM fgtracker;
GRANT ALL ON SCHEMA public TO fgtracker;
GRANT ALL ON SCHEMA public TO PUBLIC;


--
-- Name: airports; Type: ACL; Schema: public; Owner: fgtracker
--

REVOKE ALL ON TABLE airports FROM PUBLIC;
REVOKE ALL ON TABLE airports FROM fgtracker;
GRANT ALL ON TABLE airports TO fgtracker;


--
-- Name: flight_plans; Type: ACL; Schema: public; Owner: fgtracker
--

REVOKE ALL ON TABLE flight_plans FROM PUBLIC;
REVOKE ALL ON TABLE flight_plans FROM fgtracker;
GRANT ALL ON TABLE flight_plans TO fgtracker;


--
-- Name: flights; Type: ACL; Schema: public; Owner: fgtracker
--

REVOKE ALL ON TABLE flights FROM PUBLIC;
REVOKE ALL ON TABLE flights FROM fgtracker;
GRANT ALL ON TABLE flights TO fgtracker;


--
-- Name: models; Type: ACL; Schema: public; Owner: fgtracker
--

REVOKE ALL ON TABLE models FROM PUBLIC;
REVOKE ALL ON TABLE models FROM fgtracker;
GRANT ALL ON TABLE models TO fgtracker;


--
-- Name: navaid_types; Type: ACL; Schema: public; Owner: fgtracker
--

REVOKE ALL ON TABLE navaid_types FROM PUBLIC;
REVOKE ALL ON TABLE navaid_types FROM fgtracker;
GRANT ALL ON TABLE navaid_types TO fgtracker;


--
-- Name: navaids; Type: ACL; Schema: public; Owner: fgtracker
--

REVOKE ALL ON TABLE navaids FROM PUBLIC;
REVOKE ALL ON TABLE navaids FROM fgtracker;
GRANT ALL ON TABLE navaids TO fgtracker;


--
-- Name: route_points; Type: ACL; Schema: public; Owner: fgtracker
--

REVOKE ALL ON TABLE route_points FROM PUBLIC;
REVOKE ALL ON TABLE route_points FROM fgtracker;
GRANT ALL ON TABLE route_points TO fgtracker;


--
-- Name: routes; Type: ACL; Schema: public; Owner: fgtracker
--

REVOKE ALL ON TABLE routes FROM PUBLIC;
REVOKE ALL ON TABLE routes FROM fgtracker;
GRANT ALL ON TABLE routes TO fgtracker;


--
-- Name: routes_id_seq; Type: ACL; Schema: public; Owner: fgtracker
--

REVOKE ALL ON SEQUENCE routes_id_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE routes_id_seq FROM fgtracker;
GRANT ALL ON SEQUENCE routes_id_seq TO fgtracker;


--
-- Name: runways; Type: ACL; Schema: public; Owner: fgtracker
--

REVOKE ALL ON TABLE runways FROM PUBLIC;
REVOKE ALL ON TABLE runways FROM fgtracker;
GRANT ALL ON TABLE runways TO fgtracker;


--
-- Name: tracker_stats; Type: ACL; Schema: public; Owner: fgtracker
--

REVOKE ALL ON TABLE tracker_stats FROM PUBLIC;
REVOKE ALL ON TABLE tracker_stats FROM fgtracker;
GRANT ALL ON TABLE tracker_stats TO fgtracker;


--
-- Name: waypoints; Type: ACL; Schema: public; Owner: fgtracker
--

REVOKE ALL ON TABLE waypoints FROM PUBLIC;
REVOKE ALL ON TABLE waypoints FROM fgtracker;
GRANT ALL ON TABLE waypoints TO fgtracker;


--
-- PostgreSQL database dump complete
--

