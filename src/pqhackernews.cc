#include "pqhackernews.hh"
#include "pqjoin.hh"
#include <sys/resource.h>
#include "json.hh"
#include "time.hh"

namespace pq {

HackernewsPopulator::HackernewsPopulator(const Json& param)
    : nusers_(param["nusers"].as_i(5)),
      karma_(param["nusers"].as_i(5)*50*10),
      articles_(1000000),
      pre_(param["narticles"].as_i(10)),
      narticles_(0), ncomments_(0) {
}

void HackernewsRunner::post_article(uint32_t author, uint32_t aid) {
    char buf[128];
    hp_.post_article(author, aid);
    sprintf(buf, "a|%05d%05d|a|%05d", author, aid, author);
    server_.insert(Str(buf, 21), Str("lalalalala", 10));
    if (hp_.log()) {
        printf("article %.20s\n", buf);
    }
}

void HackernewsRunner::post_comment(uint32_t commentor, uint32_t author, uint32_t aid) {
    char buf[128];
    sprintf(buf, "a|%05d%05d|c|%05d|%05d", author, aid, hp_.next_comment(), commentor);
    server_.insert(Str(buf, 27), Str("lalalalala", 10));
    if (hp_.log()) {
        printf("  %.26s\n", buf);
    }
}

bool HackernewsRunner::vote(uint32_t voter, uint32_t author, uint32_t aid) {
    char buf[128];
    if (hp_.vote(aid, voter)) {
        sprintf(buf, "a|%05d%05d|v|%05d", author, aid, voter);
        server_.insert(Str(buf, 21), Str("1", 1));
        if (hp_.log()) {
            printf("vote %.20s\n", buf);
        }
        return true;
    }
    return false;
}

void HackernewsRunner::read_article(uint32_t aid, uint32_t* author) {
    char buf1[128], buf2[128];
    mandatory_assert(aid < hp_.narticles());
    *author = hp_.articles()[aid];
    sprintf(buf1, "a|%05d%05d|", *author, aid);
    sprintf(buf2, "a|%05d%05d}", *author, aid);
    auto bit = server_.lower_bound(Str(buf1, 17)),
        eit = server_.lower_bound(Str(buf2, 17));
    if (hp_.log()) {
        std::cout << ": scan [" << buf1 << "," << buf2 << ")\n";
        for (; bit != eit; ++bit)
            std::cout << "  " << bit->key() << ": " << bit->value() << "\n";
    }
    // TODO:  For each comment user, get karma
    // TODO:  Measure performance of alternative ma| materialization
}

void HackernewsRunner::populate() {
    boost::mt19937 gen;
    gen.seed(13918);
    boost::random_number_generator<boost::mt19937> rng(gen);
    uint32_t nv = 0;
    uint32_t nc = 0;
    
    for (uint32_t aid = 0; aid < hp_.pre(); aid++) {
        const uint32_t author = rng(hp_.nusers());
        post_article(author, aid);
        const uint32_t ncomment = rng(10);
        for (uint32_t j = 1; j <= ncomment; ++j) {
            nc++;
            const uint32_t commentor = rng(hp_.nusers());
            post_comment(commentor, author, aid);
        }
        const uint32_t nvote = rng(10);
        for (uint32_t j = 0; j < nvote; ++j) {
            const uint32_t voter = rng(hp_.nusers());
            if (vote(voter, author, aid))
                nv++;
        }
    }
    pq::Join* j = new pq::Join;
    bool valid = j->assign_parse("k|<author:5> "
                                 "a|<aid:10>|a|<author> "
                                 "a|<aid>|v|<voter:5>");
    mandatory_assert(valid && "Invalid join");
    j->set_jvt(jvt_count_match);
    server_.add_join("k|", "k}", j);
    std::cout << "Added " << hp_.nusers() << " users, " << hp_.narticles() 
              << " articles, " << nv << " votes, " << nc << " comments." << std::endl;
}

void HackernewsRunner::run() {
    boost::mt19937 gen;
    gen.seed(13918);
    boost::random_number_generator<boost::mt19937> rng(gen);
    const uint32_t nops = 1000;
    const uint32_t nusers = hp_.nusers();
    const uint32_t narticles = hp_.narticles();
    struct rusage ru[2];
    uint32_t nread = 0, npost = 0, ncomment = 0, nvote = 0;

    char buf1[128], buf2[128];
    sprintf(buf1, "k|");
    sprintf(buf2, "k}");
    server_.validate(Str(buf1, 2), Str(buf2, 2));

    if (hp_.log()) {
        std::cout << ": scan [" << buf1 << "," << buf2 << ")\n";
        auto bit = server_.lower_bound(Str(buf1, 2)),
            eit = server_.lower_bound(Str(buf2, 2));
        for (; bit != eit; ++bit)
            std::cout << "  " << bit->key() << ": " << bit->value() << "\n";
    }

    getrusage(RUSAGE_SELF, &ru[0]);
    for (uint32_t i = 0; i < nops; ++i) {
        uint32_t a = rng(100);
        uint32_t u = rng(nusers);
        if (a < 3) {
            post_article(u, hp_.next_aid());
            npost++;
        } else {
            uint32_t aid = rng(narticles);
            uint32_t author;
            read_article(aid, &author);
            nread++;
            if (a < 10) {
                vote(u, author, aid);
                nvote++;
            }
            if (a < 12) {
                post_comment(u, author, aid);
                ncomment++;
            }
        }
    }
    getrusage(RUSAGE_SELF, &ru[1]);
    Json stats = Json().set("nread", nread).set("npost", npost)
	.set("ncomment", ncomment).set("nvote", nvote)
	.set("time", to_real(ru[1].ru_utime - ru[0].ru_utime));
    stats.merge(server_.stats());
    std::cout << stats.unparse(Json::indent_depth(4)) << "\n";

}

}
