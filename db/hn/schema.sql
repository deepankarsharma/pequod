--
-- PostgreSQL database dump
--

SET statement_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SET check_function_bodies = false;
SET client_min_messages = warning;

--
-- Name: plpgsql; Type: EXTENSION; Schema: -; Owner: 
--

CREATE EXTENSION IF NOT EXISTS plpgsql WITH SCHEMA pg_catalog;


--
-- Name: EXTENSION plpgsql; Type: COMMENT; Schema: -; Owner: 
--

--COMMENT ON EXTENSION plpgsql IS 'PL/pgSQL procedural language';


SET search_path = public, pg_catalog;

SET default_tablespace = '';

SET default_with_oids = false;

--
-- Name: articles; Type: TABLE; Schema: public; Owner: neha; Tablespace: 
--

DROP TABLE IF EXISTS articles CASCADE;

CREATE TABLE articles (
    aid integer NOT NULL,
    author integer,
    link text
);


--ALTER TABLE public.articles OWNER TO neha;

--
-- Name: comments; Type: TABLE; Schema: public; Owner: neha; Tablespace: 
--

DROP TABLE IF EXISTS comments CASCADE;

CREATE TABLE comments (
    cid integer NOT NULL,
    aid integer,
    commenter integer,
    comment text
);

CREATE INDEX comments_aid ON comments (aid);
CREATE INDEX comments_commenter ON comments (commenter);


--ALTER TABLE public.comments OWNER TO neha;

--
-- Name: votes; Type: TABLE; Schema: public; Owner: neha; Tablespace: 
--

DROP TABLE IF EXISTS votes CASCADE;

CREATE TABLE votes (
    aid integer,
    voter integer
);

CREATE INDEX votes_aid ON votes (aid);

--ALTER TABLE public.votes OWNER TO neha;

--
-- Data for Name: articles; Type: TABLE DATA; Schema: public; Owner: neha
--



--
-- Data for Name: comments; Type: TABLE DATA; Schema: public; Owner: neha
--

--
-- Data for Name: votes; Type: TABLE DATA; Schema: public; Owner: neha
--


--
-- Name: articles_pkey; Type: CONSTRAINT; Schema: public; Owner: neha; Tablespace: 
--

ALTER TABLE ONLY articles
    ADD CONSTRAINT articles_pkey PRIMARY KEY (aid);


--
-- Name: comments_pkey; Type: CONSTRAINT; Schema: public; Owner: neha; Tablespace: 
--

ALTER TABLE ONLY comments
    ADD CONSTRAINT comments_pkey PRIMARY KEY (cid);


--
-- Name: public; Type: ACL; Schema: -; Owner: postgres
--

REVOKE ALL ON SCHEMA public FROM PUBLIC;
REVOKE ALL ON SCHEMA public FROM postgres;
GRANT ALL ON SCHEMA public TO postgres;
GRANT ALL ON SCHEMA public TO PUBLIC;


--
-- PostgreSQL database dump complete
--

