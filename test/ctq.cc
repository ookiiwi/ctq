#include <string>
#include <vector>
#include <cstring>

#include "catch2/catch_test_macros.hpp"
#include "ctq_writer.h"
#include "ctq_reader.h"
#include "ctq_util.hh"


// regexp
// blank (?<=>)\s+|\n\s*(?=<)
// quote ((?<==)")|((?<=[^\\])"(?=>))

TEST_CASE("2d array") {
    std::vector<std::vector<int>> vec{ { 0,1,2,3 }, { 0,1,2 }, { 0,1,2,3,4,5 }, { 0,1 } };
    Contiguous2dArray<int> cvec(vec);
    std::stringstream ss;

    cvec.save(ss);
    Contiguous2dArray<int> cvecIO(ss);

    for (int i = 0; i < vec.size(); ++i) {
        REQUIRE(cvec[i] == vec[i]);
        REQUIRE(cvecIO[i] == vec[i]);
    }
}

TEST_CASE("simple") {
    const std::string input_filename  = "dataset/simple.tei";
    const std::string output_filename = "dataset/simple.ctq";
    const std::vector<std::string> keys{ "袱紗", "膨よか", "ふしだら", "嗚呼" };
    const std::vector<uint64_t>    ids { 1010990, 1011000, 1011010, 1565440 };
    const std::vector<std::string> entries{ 
        "<entry><form type=\"k_ele\"><orth>袱紗</orth></form><form type=\"k_ele\"><orth>帛紗</orth></form><form type=\"k_ele\"><orth>服紗</orth></form><form type=\"r_ele\"><orth>ふくさ</orth></form><sense><note type=\"pos\">noun (common) (futsuumeishi)</note><cit type=\"trans\"><quote>small silk wrapper</quote></cit><cit type=\"trans\"><quote>small cloth for wiping tea utensils</quote></cit><cit type=\"trans\"><quote>crepe wrapper</quote></cit></sense></entry>",
        "<entry><form type=\"k_ele\"><orth>膨よか</orth><lbl type=\"ke_inf\">rarely-used kanji form</lbl></form><form type=\"k_ele\"><orth>脹よか</orth><lbl type=\"ke_inf\">rarely-used kanji form</lbl></form><form type=\"r_ele\"><orth>ふくよか</orth></form><sense><note type=\"pos\">adjectival nouns or quasi-adjectives (keiyodoshi)</note><note>word usually written using kana alone</note><cit type=\"trans\"><quote>plump</quote></cit><cit type=\"trans\"><quote>fleshy</quote></cit><cit type=\"trans\"><quote>well-rounded</quote></cit><cit type=\"trans\"><quote>full</quote></cit></sense><sense><note type=\"pos\">adjectival nouns or quasi-adjectives (keiyodoshi)</note><note>word usually written using kana alone</note><cit type=\"trans\"><quote>pleasant (fragrance)</quote></cit><cit type=\"trans\"><quote>rich</quote></cit></sense></entry>",
        "<entry><form type=\"r_ele\"><orth>ふしだら</orth></form><sense><note type=\"pos\">adjectival nouns or quasi-adjectives (keiyodoshi)</note><note type=\"pos\">noun (common) (futsuumeishi)</note><cit type=\"trans\"><quote>loose</quote></cit><cit type=\"trans\"><quote>immoral</quote></cit><cit type=\"trans\"><quote>dissolute</quote></cit><cit type=\"trans\"><quote>dissipated</quote></cit><cit type=\"trans\"><quote>licentious</quote></cit><cit type=\"trans\"><quote>fast</quote></cit></sense><sense><note type=\"pos\">adjectival nouns or quasi-adjectives (keiyodoshi)</note><note type=\"pos\">noun (common) (futsuumeishi)</note><cit type=\"trans\"><quote>slovenly</quote></cit><cit type=\"trans\"><quote>untidy</quote></cit><cit type=\"trans\"><quote>messy</quote></cit></sense></entry>",
        "<entry><form type=\"k_ele\"><orth>嗚呼</orth><lbl type=\"ke_inf\">ateji (phonetic) reading</lbl></form><form type=\"k_ele\"><orth>噫</orth><lbl type=\"ke_inf\">ateji (phonetic) reading</lbl><lbl type=\"ke_inf\">rarely-used kanji form</lbl></form><form type=\"k_ele\"><orth>嗟</orth><lbl type=\"ke_inf\">search-only kanji form</lbl></form><form type=\"k_ele\"><orth>於乎</orth><lbl type=\"ke_inf\">search-only kanji form</lbl></form><form type=\"k_ele\"><orth>於戯</orth><lbl type=\"ke_inf\">search-only kanji form</lbl></form><form type=\"k_ele\"><orth>嗟乎</orth><lbl type=\"ke_inf\">search-only kanji form</lbl></form><form type=\"k_ele\"><orth>吁</orth><lbl type=\"ke_inf\">search-only kanji form</lbl></form><form type=\"r_ele\"><orth>ああ</orth><usg type=\"re_pri\">spec1</usg></form><form type=\"r_ele\"><orth>あー</orth><lbl type=\"re_nokanji\"></lbl><usg type=\"re_pri\">spec1</usg></form><form type=\"r_ele\"><orth>あぁ</orth><lbl type=\"re_inf\">search-only kana form</lbl></form><form type=\"r_ele\"><orth>アー</orth><lbl type=\"re_inf\">search-only kana form</lbl></form><form type=\"r_ele\"><orth>アア</orth><lbl type=\"re_inf\">search-only kana form</lbl></form><form type=\"r_ele\"><orth>アァ</orth><lbl type=\"re_inf\">search-only kana form</lbl></form><sense><note type=\"pos\">interjection (kandoushi)</note><note>word usually written using kana alone</note><cit type=\"trans\"><quote>ah!</quote></cit><cit type=\"trans\"><quote>oh!</quote></cit><cit type=\"trans\"><quote>alas!</quote></cit></sense><sense><note type=\"pos\">interjection (kandoushi)</note><note>word usually written using kana alone</note><cit type=\"trans\"><quote>yes</quote></cit><cit type=\"trans\"><quote>indeed</quote></cit><cit type=\"trans\"><quote>that is correct</quote></cit></sense><sense><note type=\"pos\">interjection (kandoushi)</note><note>word usually written using kana alone</note><note type=\"sense\">in exasperation</note><cit type=\"trans\"><quote>aah</quote></cit><cit type=\"trans\"><quote>gah</quote></cit><cit type=\"trans\"><quote>argh</quote></cit></sense><sense><note type=\"pos\">interjection (kandoushi)</note><note>word usually written using kana alone</note><cit type=\"trans\"><quote>hey!</quote></cit><cit type=\"trans\"><quote>yo!</quote></cit></sense><sense><note type=\"pos\">interjection (kandoushi)</note><note>word usually written using kana alone</note><cit type=\"trans\"><quote>uh huh</quote></cit><cit type=\"trans\"><quote>yeah yeah</quote></cit><cit type=\"trans\"><quote>right</quote></cit><cit type=\"trans\"><quote>gotcha</quote></cit></sense></entry>"
    };
    const std::vector<std::string> paths {
        "/entry/form/orth",         // find
        "/entry/sense/cit/quote",   // don't find
        "/entry/sense/note"
    };

    CTQ::write(input_filename, output_filename, paths);

    SECTION("C++") {
        CTQ::Reader reader(output_filename, true);

        for (int i = 0; i < keys.size(); ++i) {
            // default
            {
                auto find_ret = reader.find(keys[i]);
                auto it = find_ret.begin();

                REQUIRE(find_ret.size() == 1);
                REQUIRE(it->first == keys[i]);
                REQUIRE(it->second.size() == 1);
                REQUIRE(it->second.front() == ids[i]);


                std::string entry = reader.get(it->second.front());
                REQUIRE(entry.size() > 0);
                REQUIRE(entry == entries[i]);
            }

            // path
            {
                auto find_orth  = reader.find(keys[i], 0, 0, 1);
                auto find_quote = reader.find(keys[i], 0, 0, 2);

                REQUIRE(find_orth.size() == 1);
                REQUIRE(find_quote.size() == 0);
            }
        }

        // range for "pleasant (fragrance)" and "plump"
        {
            auto find1 = reader.find("p%", 0, 1);
            auto find2 = reader.find("p%", 1, 1);

            REQUIRE(find1.size() == 1);
            REQUIRE(find2.size() == 0);
        }

        // prefix for same entry
        {
            auto find = reader.find("p%");

            REQUIRE(find.size() == 1);
        }

        // empty keyword
        {
            auto find = reader.find("");

            REQUIRE(find.size() == 0);
        }

        // filter
        {
            auto find = reader.find("noun%", 0, 0, 0, "袱紗");
            auto find2 = reader.find("noun%",  0, 0, 0, "袱紗", 2);
            auto find3 = reader.find("noun%",  0, 0, 0, "fdsfsdsd");

            REQUIRE(find.size() == 1);
            REQUIRE(find.begin()->first == "noun (common) (futsuumeishi)");

            REQUIRE(find2.size() == 0);
            REQUIRE(find3.size() == 0);
        }
    }

    SECTION("C") {
        //SKIP("");
        ctq_ctx *ctx = ctq_create_reader(output_filename.c_str());

        for (int i = 0; i < keys.size(); ++i) {
            // default
            {
                ctq_find_ret *arr = ctq_find(ctx, keys[i].c_str(), 0, 0, 0, "", 0);
                
                REQUIRE(arr != NULL);
                REQUIRE(std::string(arr[0].key) == keys[i]);
                REQUIRE(arr[0].id_cnt == 1);
                REQUIRE(arr[0].ids[0] == ids[i]);


                const char *entry = ctq_get(ctx, arr[0].ids[0]);
                REQUIRE(entry != NULL);
                REQUIRE(strlen(entry) > 0);
                REQUIRE(std::string(entry) == entries[i]);
                free((void*)entry);
            }
        }

        {
            ctq_find_ret *arr = ctq_find(ctx, "p%", 0, 0, 0, "", 0);

            REQUIRE(arr != NULL);

            ctq_find_ret_free(arr);
        }

        // empty keyword
        {
            ctq_find_ret *arr = ctq_find(ctx, "", 0, 0, 0, "", 0);

            REQUIRE(arr == NULL);
        }

        // filter
        {
            ctq_find_ret *find  = ctq_find(ctx, "noun%", 0, 0, 0, "袱紗", 0);
            ctq_find_ret *find2 = ctq_find(ctx, "noun%", 0, 0, 0, "袱紗", 2);
            ctq_find_ret *find3 = ctq_find(ctx, "noun%", 0, 0, 0, "fdsfsdsd", 0);

            REQUIRE(find != NULL);
            REQUIRE(std::string(find[0].key) == "noun (common) (futsuumeishi)");

            REQUIRE(find2 == NULL);
            REQUIRE(find3 == NULL);

            ctq_find_ret_free(find);
        }

        ctq_destroy_reader(ctx);
    }
}