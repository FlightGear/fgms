--
-- PostgreSQL database dump
--

SET client_encoding = 'LATIN2';
SET standard_conforming_strings = off;
SET check_function_bodies = false;
SET client_min_messages = warning;
SET escape_string_warning = off;

--
-- Name: SCHEMA public; Type: COMMENT; Schema: -; Owner: psql
--

COMMENT ON SCHEMA public IS 'Standard public schema';


SET search_path = public, pg_catalog;

--
-- Name: gtrgm; Type: SHELL TYPE; Schema: public; Owner: root
--

CREATE TYPE gtrgm;


--
-- Name: gtrgm_in(cstring); Type: FUNCTION; Schema: public; Owner: root
--

CREATE FUNCTION gtrgm_in(cstring) RETURNS gtrgm
    AS '/opt/postgresql/lib/libpg_trgm.so', 'gtrgm_in'
    LANGUAGE c STRICT;


ALTER FUNCTION public.gtrgm_in(cstring) OWNER TO root;

--
-- Name: gtrgm_out(gtrgm); Type: FUNCTION; Schema: public; Owner: root
--

CREATE FUNCTION gtrgm_out(gtrgm) RETURNS cstring
    AS '/opt/postgresql/lib/libpg_trgm.so', 'gtrgm_out'
    LANGUAGE c STRICT;


ALTER FUNCTION public.gtrgm_out(gtrgm) OWNER TO root;

--
-- Name: gtrgm; Type: TYPE; Schema: public; Owner: root
--

CREATE TYPE gtrgm (
    INTERNALLENGTH = variable,
    INPUT = gtrgm_in,
    OUTPUT = gtrgm_out,
    ALIGNMENT = int4,
    STORAGE = plain
);


ALTER TYPE public.gtrgm OWNER TO root;

--
-- Name: gtrgm_compress(internal); Type: FUNCTION; Schema: public; Owner: root
--

CREATE FUNCTION gtrgm_compress(internal) RETURNS internal
    AS '/opt/postgresql/lib/libpg_trgm.so', 'gtrgm_compress'
    LANGUAGE c;


ALTER FUNCTION public.gtrgm_compress(internal) OWNER TO root;

--
-- Name: gtrgm_consistent(gtrgm, internal, integer); Type: FUNCTION; Schema: public; Owner: root
--

CREATE FUNCTION gtrgm_consistent(gtrgm, internal, integer) RETURNS boolean
    AS '/opt/postgresql/lib/libpg_trgm.so', 'gtrgm_consistent'
    LANGUAGE c;


ALTER FUNCTION public.gtrgm_consistent(gtrgm, internal, integer) OWNER TO root;

--
-- Name: gtrgm_decompress(internal); Type: FUNCTION; Schema: public; Owner: root
--

CREATE FUNCTION gtrgm_decompress(internal) RETURNS internal
    AS '/opt/postgresql/lib/libpg_trgm.so', 'gtrgm_decompress'
    LANGUAGE c;


ALTER FUNCTION public.gtrgm_decompress(internal) OWNER TO root;

--
-- Name: gtrgm_penalty(internal, internal, internal); Type: FUNCTION; Schema: public; Owner: root
--

CREATE FUNCTION gtrgm_penalty(internal, internal, internal) RETURNS internal
    AS '/opt/postgresql/lib/libpg_trgm.so', 'gtrgm_penalty'
    LANGUAGE c STRICT;


ALTER FUNCTION public.gtrgm_penalty(internal, internal, internal) OWNER TO root;

--
-- Name: gtrgm_picksplit(internal, internal); Type: FUNCTION; Schema: public; Owner: root
--

CREATE FUNCTION gtrgm_picksplit(internal, internal) RETURNS internal
    AS '/opt/postgresql/lib/libpg_trgm.so', 'gtrgm_picksplit'
    LANGUAGE c;


ALTER FUNCTION public.gtrgm_picksplit(internal, internal) OWNER TO root;

--
-- Name: gtrgm_same(gtrgm, gtrgm, internal); Type: FUNCTION; Schema: public; Owner: root
--

CREATE FUNCTION gtrgm_same(gtrgm, gtrgm, internal) RETURNS internal
    AS '/opt/postgresql/lib/libpg_trgm.so', 'gtrgm_same'
    LANGUAGE c;


ALTER FUNCTION public.gtrgm_same(gtrgm, gtrgm, internal) OWNER TO root;

--
-- Name: gtrgm_union(bytea, internal); Type: FUNCTION; Schema: public; Owner: root
--

CREATE FUNCTION gtrgm_union(bytea, internal) RETURNS integer[]
    AS '/opt/postgresql/lib/libpg_trgm.so', 'gtrgm_union'
    LANGUAGE c;


ALTER FUNCTION public.gtrgm_union(bytea, internal) OWNER TO root;

--
-- Name: set_limit(real); Type: FUNCTION; Schema: public; Owner: root
--

CREATE FUNCTION set_limit(real) RETURNS real
    AS '/opt/postgresql/lib/libpg_trgm.so', 'set_limit'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.set_limit(real) OWNER TO root;

--
-- Name: show_limit(); Type: FUNCTION; Schema: public; Owner: root
--

CREATE FUNCTION show_limit() RETURNS real
    AS '/opt/postgresql/lib/libpg_trgm.so', 'show_limit'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.show_limit() OWNER TO root;

--
-- Name: show_trgm(text); Type: FUNCTION; Schema: public; Owner: root
--

CREATE FUNCTION show_trgm(text) RETURNS text[]
    AS '/opt/postgresql/lib/libpg_trgm.so', 'show_trgm'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.show_trgm(text) OWNER TO root;

--
-- Name: similarity(text, text); Type: FUNCTION; Schema: public; Owner: root
--

CREATE FUNCTION similarity(text, text) RETURNS real
    AS '/opt/postgresql/lib/libpg_trgm.so', 'similarity'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.similarity(text, text) OWNER TO root;

--
-- Name: similarity_op(text, text); Type: FUNCTION; Schema: public; Owner: root
--

CREATE FUNCTION similarity_op(text, text) RETURNS boolean
    AS '/opt/postgresql/lib/libpg_trgm.so', 'similarity_op'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.similarity_op(text, text) OWNER TO root;

--
-- Name: %; Type: OPERATOR; Schema: public; Owner: root
--

CREATE OPERATOR % (
    PROCEDURE = similarity_op,
    LEFTARG = text,
    RIGHTARG = text,
    COMMUTATOR = %,
    RESTRICT = contsel,
    JOIN = contjoinsel
);


ALTER OPERATOR public.% (text, text) OWNER TO root;

--
-- Name: gist_trgm_ops; Type: OPERATOR CLASS; Schema: public; Owner: root
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


ALTER OPERATOR CLASS public.gist_trgm_ops USING gist OWNER TO root;

SET default_tablespace = '';

SET default_with_oids = false;

--
-- Name: airports; Type: TABLE; Schema: public; Owner: tgbp; Tablespace: 
--

CREATE TABLE airports (
    id integer NOT NULL,
    icao text,
    name text
);


ALTER TABLE public.airports OWNER TO tgbp;

--
-- Name: airports_id_seq; Type: SEQUENCE; Schema: public; Owner: tgbp
--

CREATE SEQUENCE airports_id_seq
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.airports_id_seq OWNER TO tgbp;

--
-- Name: airports_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: tgbp
--

ALTER SEQUENCE airports_id_seq OWNED BY airports.id;


--
-- Name: flights; Type: TABLE; Schema: public; Owner: tgbp; Tablespace: 
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


ALTER TABLE public.flights OWNER TO tgbp;

--
-- Name: delme; Type: VIEW; Schema: public; Owner: tgbp
--

CREATE VIEW delme AS
    SELECT 'S' AS "type", flights.start_time AS "time" FROM flights WHERE (flights.status = 'CLOSED'::text) UNION ALL SELECT 'E' AS "type", flights.end_time AS "time" FROM flights WHERE (flights.status = 'CLOSED'::text);


ALTER TABLE public.delme OWNER TO tgbp;

--
-- Name: fixes; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE fixes (
    id integer NOT NULL,
    latitude double precision,
    longitude double precision,
    name text
);


ALTER TABLE public.fixes OWNER TO root;

--
-- Name: fixes_id_seq; Type: SEQUENCE; Schema: public; Owner: root
--

CREATE SEQUENCE fixes_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.fixes_id_seq OWNER TO root;

--
-- Name: fixes_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: root
--

ALTER SEQUENCE fixes_id_seq OWNED BY fixes.id;


--
-- Name: flight_plans; Type: TABLE; Schema: public; Owner: tgbp; Tablespace: 
--

CREATE TABLE flight_plans (
    id integer,
    seq integer,
    fix_name text
);


ALTER TABLE public.flight_plans OWNER TO tgbp;

--
-- Name: flights_id_seq; Type: SEQUENCE; Schema: public; Owner: tgbp
--

CREATE SEQUENCE flights_id_seq
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.flights_id_seq OWNER TO tgbp;

--
-- Name: flights_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: tgbp
--

ALTER SEQUENCE flights_id_seq OWNED BY flights.id;


--
-- Name: flights_old; Type: TABLE; Schema: public; Owner: root; Tablespace: 
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


ALTER TABLE public.flights_old OWNER TO root;

--
-- Name: models; Type: TABLE; Schema: public; Owner: tgbp; Tablespace: 
--

CREATE TABLE models (
    fg_string text NOT NULL,
    human_string text
);


ALTER TABLE public.models OWNER TO tgbp;

--
-- Name: navaid_types; Type: TABLE; Schema: public; Owner: tgbp; Tablespace: 
--

CREATE TABLE navaid_types (
    id integer NOT NULL,
    label text
);


ALTER TABLE public.navaid_types OWNER TO tgbp;

--
-- Name: navaids; Type: TABLE; Schema: public; Owner: tgbp; Tablespace: 
--

CREATE TABLE navaids (
    id integer NOT NULL,
    "type" integer,
    latitude double precision,
    longitude double precision,
    range integer,
    freq double precision,
    ilt text,
    name text
);


ALTER TABLE public.navaids OWNER TO tgbp;

--
-- Name: navaids_id_seq; Type: SEQUENCE; Schema: public; Owner: tgbp
--

CREATE SEQUENCE navaids_id_seq
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.navaids_id_seq OWNER TO tgbp;

--
-- Name: navaids_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: tgbp
--

ALTER SEQUENCE navaids_id_seq OWNED BY navaids.id;


--
-- Name: route_points; Type: TABLE; Schema: public; Owner: root; Tablespace: 
--

CREATE TABLE route_points (
    route_id integer,
    seq integer,
    navaid_id integer
);


ALTER TABLE public.route_points OWNER TO root;

--
-- Name: routes; Type: TABLE; Schema: public; Owner: root; Tablespace: 
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


ALTER TABLE public.routes OWNER TO root;

--
-- Name: routes_id_seq; Type: SEQUENCE; Schema: public; Owner: root
--

CREATE SEQUENCE routes_id_seq
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.routes_id_seq OWNER TO root;

--
-- Name: routes_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: root
--

ALTER SEQUENCE routes_id_seq OWNED BY routes.id;


--
-- Name: runways; Type: TABLE; Schema: public; Owner: tgbp; Tablespace: 
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


ALTER TABLE public.runways OWNER TO tgbp;

--
-- Name: tracker_stats; Type: TABLE; Schema: public; Owner: tgbp; Tablespace: 
--

CREATE TABLE tracker_stats (
    "month" text NOT NULL,
    count integer,
    modified timestamp without time zone
);


ALTER TABLE public.tracker_stats OWNER TO tgbp;

--
-- Name: waypoints; Type: TABLE; Schema: public; Owner: tgbp; Tablespace: 
--

CREATE TABLE waypoints (
    id integer NOT NULL,
    flight_id integer,
    "time" timestamp with time zone,
    latitude double precision,
    longitude double precision,
    altitude double precision
);


ALTER TABLE public.waypoints OWNER TO tgbp;

--
-- Name: waypoints_id_seq; Type: SEQUENCE; Schema: public; Owner: tgbp
--

CREATE SEQUENCE waypoints_id_seq
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.waypoints_id_seq OWNER TO tgbp;

--
-- Name: waypoints_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: tgbp
--

ALTER SEQUENCE waypoints_id_seq OWNED BY waypoints.id;


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: tgbp
--

ALTER TABLE airports ALTER COLUMN id SET DEFAULT nextval('airports_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: root
--

ALTER TABLE fixes ALTER COLUMN id SET DEFAULT nextval('fixes_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: tgbp
--

ALTER TABLE flights ALTER COLUMN id SET DEFAULT nextval('flights_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: tgbp
--

ALTER TABLE navaids ALTER COLUMN id SET DEFAULT nextval('navaids_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: root
--

ALTER TABLE routes ALTER COLUMN id SET DEFAULT nextval('routes_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: tgbp
--

ALTER TABLE waypoints ALTER COLUMN id SET DEFAULT nextval('waypoints_id_seq'::regclass);


--
-- Name: airports_pkey; Type: CONSTRAINT; Schema: public; Owner: tgbp; Tablespace: 
--

ALTER TABLE ONLY airports
    ADD CONSTRAINT airports_pkey PRIMARY KEY (id);


--
-- Name: flights_pkey; Type: CONSTRAINT; Schema: public; Owner: tgbp; Tablespace: 
--

ALTER TABLE ONLY flights
    ADD CONSTRAINT flights_pkey PRIMARY KEY (id);


--
-- Name: models_pkey; Type: CONSTRAINT; Schema: public; Owner: tgbp; Tablespace: 
--

ALTER TABLE ONLY models
    ADD CONSTRAINT models_pkey PRIMARY KEY (fg_string);


--
-- Name: navaid_types_pkey; Type: CONSTRAINT; Schema: public; Owner: tgbp; Tablespace: 
--

ALTER TABLE ONLY navaid_types
    ADD CONSTRAINT navaid_types_pkey PRIMARY KEY (id);


--
-- Name: tracker_stats_pkey; Type: CONSTRAINT; Schema: public; Owner: tgbp; Tablespace: 
--

ALTER TABLE ONLY tracker_stats
    ADD CONSTRAINT tracker_stats_pkey PRIMARY KEY ("month");


--
-- Name: waypoints_pkey; Type: CONSTRAINT; Schema: public; Owner: tgbp; Tablespace: 
--

ALTER TABLE ONLY waypoints
    ADD CONSTRAINT waypoints_pkey PRIMARY KEY (id);


--
-- Name: airports_icao_idx; Type: INDEX; Schema: public; Owner: tgbp; Tablespace: 
--

CREATE UNIQUE INDEX airports_icao_idx ON airports USING btree (icao);


--
-- Name: fixes-id-idx; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE UNIQUE INDEX "fixes-id-idx" ON fixes USING btree (id);


--
-- Name: flight_plans-id-idx; Type: INDEX; Schema: public; Owner: tgbp; Tablespace: 
--

CREATE INDEX "flight_plans-id-idx" ON flight_plans USING btree (id);


--
-- Name: flight_plans-seq-idx; Type: INDEX; Schema: public; Owner: tgbp; Tablespace: 
--

CREATE INDEX "flight_plans-seq-idx" ON flight_plans USING btree (seq);


--
-- Name: flights-times-idx; Type: INDEX; Schema: public; Owner: tgbp; Tablespace: 
--

CREATE INDEX "flights-times-idx" ON flights USING btree (start_time, end_time);


--
-- Name: flights_callsign; Type: INDEX; Schema: public; Owner: tgbp; Tablespace: 
--

CREATE INDEX flights_callsign ON flights USING btree (callsign);


--
-- Name: flights_status; Type: INDEX; Schema: public; Owner: tgbp; Tablespace: 
--

CREATE INDEX flights_status ON flights USING btree (status);


--
-- Name: navaids-id-idx; Type: INDEX; Schema: public; Owner: tgbp; Tablespace: 
--

CREATE INDEX "navaids-id-idx" ON navaids USING btree (id);


--
-- Name: navaids-ilt-idx; Type: INDEX; Schema: public; Owner: tgbp; Tablespace: 
--

CREATE INDEX "navaids-ilt-idx" ON navaids USING btree (ilt);


--
-- Name: navaids-lat_lon-idx; Type: INDEX; Schema: public; Owner: tgbp; Tablespace: 
--

CREATE INDEX "navaids-lat_lon-idx" ON navaids USING btree (latitude, longitude);


--
-- Name: navaids_type_idx; Type: INDEX; Schema: public; Owner: tgbp; Tablespace: 
--

CREATE INDEX navaids_type_idx ON navaids USING btree ("type");


--
-- Name: route_points-route_id-idx; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE INDEX "route_points-route_id-idx" ON route_points USING btree (route_id);


--
-- Name: route_points-seq-idx; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE INDEX "route_points-seq-idx" ON route_points USING btree (seq);


--
-- Name: routes-arr-idx; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE INDEX "routes-arr-idx" ON routes USING btree (arr_airport_id, arr_runway);


--
-- Name: routes-dep-idx; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE INDEX "routes-dep-idx" ON routes USING btree (dep_airport_id, dep_runway);


--
-- Name: routes-id-idx; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE INDEX "routes-id-idx" ON routes USING btree (id);


--
-- Name: routes-limits-idx; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE INDEX "routes-limits-idx" ON routes USING btree (types, mind, maxd);


--
-- Name: routes-status-idx; Type: INDEX; Schema: public; Owner: root; Tablespace: 
--

CREATE INDEX "routes-status-idx" ON routes USING btree (status);


--
-- Name: runways-airport_id-idx; Type: INDEX; Schema: public; Owner: tgbp; Tablespace: 
--

CREATE INDEX "runways-airport_id-idx" ON runways USING btree (airport_id);


--
-- Name: waypoints-flight_id-idx; Type: INDEX; Schema: public; Owner: tgbp; Tablespace: 
--

CREATE INDEX "waypoints-flight_id-idx" ON waypoints USING btree (flight_id);


--
-- Name: waypoints-time-idx; Type: INDEX; Schema: public; Owner: tgbp; Tablespace: 
--

CREATE INDEX "waypoints-time-idx" ON waypoints USING btree ("time");


--
-- Name: public; Type: ACL; Schema: -; Owner: psql
--

REVOKE ALL ON SCHEMA public FROM PUBLIC;
REVOKE ALL ON SCHEMA public FROM psql;
GRANT ALL ON SCHEMA public TO psql;
GRANT ALL ON SCHEMA public TO PUBLIC;


--
-- Name: airports; Type: ACL; Schema: public; Owner: tgbp
--

REVOKE ALL ON TABLE airports FROM PUBLIC;
REVOKE ALL ON TABLE airports FROM tgbp;
GRANT ALL ON TABLE airports TO tgbp;
GRANT SELECT ON TABLE airports TO httpd;


--
-- Name: flights; Type: ACL; Schema: public; Owner: tgbp
--

REVOKE ALL ON TABLE flights FROM PUBLIC;
REVOKE ALL ON TABLE flights FROM tgbp;
GRANT ALL ON TABLE flights TO tgbp;
GRANT SELECT ON TABLE flights TO httpd;


--
-- Name: flight_plans; Type: ACL; Schema: public; Owner: tgbp
--

REVOKE ALL ON TABLE flight_plans FROM PUBLIC;
REVOKE ALL ON TABLE flight_plans FROM tgbp;
GRANT ALL ON TABLE flight_plans TO tgbp;
GRANT SELECT ON TABLE flight_plans TO httpd;


--
-- Name: models; Type: ACL; Schema: public; Owner: tgbp
--

REVOKE ALL ON TABLE models FROM PUBLIC;
REVOKE ALL ON TABLE models FROM tgbp;
GRANT ALL ON TABLE models TO tgbp;
GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE models TO httpd;


--
-- Name: navaid_types; Type: ACL; Schema: public; Owner: tgbp
--

REVOKE ALL ON TABLE navaid_types FROM PUBLIC;
REVOKE ALL ON TABLE navaid_types FROM tgbp;
GRANT ALL ON TABLE navaid_types TO tgbp;
GRANT SELECT ON TABLE navaid_types TO httpd;


--
-- Name: navaids; Type: ACL; Schema: public; Owner: tgbp
--

REVOKE ALL ON TABLE navaids FROM PUBLIC;
REVOKE ALL ON TABLE navaids FROM tgbp;
GRANT ALL ON TABLE navaids TO tgbp;
GRANT SELECT ON TABLE navaids TO httpd;


--
-- Name: route_points; Type: ACL; Schema: public; Owner: root
--

REVOKE ALL ON TABLE route_points FROM PUBLIC;
REVOKE ALL ON TABLE route_points FROM root;
GRANT ALL ON TABLE route_points TO root;
GRANT ALL ON TABLE route_points TO tgbp;
GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE route_points TO httpd;


--
-- Name: routes; Type: ACL; Schema: public; Owner: root
--

REVOKE ALL ON TABLE routes FROM PUBLIC;
REVOKE ALL ON TABLE routes FROM root;
GRANT ALL ON TABLE routes TO root;
GRANT ALL ON TABLE routes TO tgbp;
GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE routes TO httpd;


--
-- Name: routes_id_seq; Type: ACL; Schema: public; Owner: root
--

REVOKE ALL ON SEQUENCE routes_id_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE routes_id_seq FROM root;
GRANT ALL ON SEQUENCE routes_id_seq TO root;
GRANT ALL ON SEQUENCE routes_id_seq TO tgbp;
GRANT SELECT,UPDATE ON SEQUENCE routes_id_seq TO httpd;


--
-- Name: runways; Type: ACL; Schema: public; Owner: tgbp
--

REVOKE ALL ON TABLE runways FROM PUBLIC;
REVOKE ALL ON TABLE runways FROM tgbp;
GRANT ALL ON TABLE runways TO tgbp;
GRANT SELECT ON TABLE runways TO httpd;


--
-- Name: tracker_stats; Type: ACL; Schema: public; Owner: tgbp
--

REVOKE ALL ON TABLE tracker_stats FROM PUBLIC;
REVOKE ALL ON TABLE tracker_stats FROM tgbp;
GRANT ALL ON TABLE tracker_stats TO tgbp;
GRANT SELECT ON TABLE tracker_stats TO httpd;


--
-- Name: waypoints; Type: ACL; Schema: public; Owner: tgbp
--

REVOKE ALL ON TABLE waypoints FROM PUBLIC;
REVOKE ALL ON TABLE waypoints FROM tgbp;
GRANT ALL ON TABLE waypoints TO tgbp;
GRANT SELECT ON TABLE waypoints TO httpd;


--
-- PostgreSQL database dump complete
--

