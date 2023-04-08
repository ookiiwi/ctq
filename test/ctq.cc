#include <string>
#include <vector>
#include <cstring>

#include "catch2/catch_test_macros.hpp"
#include "ctq_writer.h"
#include "ctq_reader.h"


// regexp
// blank (?<=>)\s+|\n\s*(?=<)
// quote ((?<==)")|((?<=[^\\])"(?=>))

TEST_CASE("simple") {
    const std::string input_filename  = "dataset/simple.tei";
    const std::string output_filename = "dataset/simple.ctq";
    const std::vector<std::string> keys{ "袱紗", "膨よか", "ふしだら" };
    const std::vector<uint64_t>    ids { 1010990, 1011000, 1011010 };
    const std::vector<std::string> entries{ 
        "<entry><form type=\"k_ele\"><orth>袱紗</orth></form><form type=\"k_ele\"><orth>帛紗</orth></form><form type=\"k_ele\"><orth>服紗</orth></form><form type=\"r_ele\"><orth>ふくさ</orth></form><sense><note type=\"pos\">noun (common) (futsuumeishi)</note><cit type=\"trans\"><quote>small silk wrapper</quote></cit><cit type=\"trans\"><quote>small cloth for wiping tea utensils</quote></cit><cit type=\"trans\"><quote>crepe wrapper</quote></cit></sense></entry>",
        "<entry><form type=\"k_ele\"><orth>膨よか</orth><lbl type=\"ke_inf\">rarely-used kanji form</lbl></form><form type=\"k_ele\"><orth>脹よか</orth><lbl type=\"ke_inf\">rarely-used kanji form</lbl></form><form type=\"r_ele\"><orth>ふくよか</orth></form><sense><note type=\"pos\">adjectival nouns or quasi-adjectives (keiyodoshi)</note><note>word usually written using kana alone</note><cit type=\"trans\"><quote>plump</quote></cit><cit type=\"trans\"><quote>fleshy</quote></cit><cit type=\"trans\"><quote>well-rounded</quote></cit><cit type=\"trans\"><quote>full</quote></cit></sense><sense><note type=\"pos\">adjectival nouns or quasi-adjectives (keiyodoshi)</note><note>word usually written using kana alone</note><cit type=\"trans\"><quote>pleasant (fragrance)</quote></cit><cit type=\"trans\"><quote>rich</quote></cit></sense></entry>",
        "<entry><form type=\"r_ele\"><orth>ふしだら</orth></form><sense><note type=\"pos\">adjectival nouns or quasi-adjectives (keiyodoshi)</note><note type=\"pos\">noun (common) (futsuumeishi)</note><cit type=\"trans\"><quote>loose</quote></cit><cit type=\"trans\"><quote>immoral</quote></cit><cit type=\"trans\"><quote>dissolute</quote></cit><cit type=\"trans\"><quote>dissipated</quote></cit><cit type=\"trans\"><quote>licentious</quote></cit><cit type=\"trans\"><quote>fast</quote></cit></sense><sense><note type=\"pos\">adjectival nouns or quasi-adjectives (keiyodoshi)</note><note type=\"pos\">noun (common) (futsuumeishi)</note><cit type=\"trans\"><quote>slovenly</quote></cit><cit type=\"trans\"><quote>untidy</quote></cit><cit type=\"trans\"><quote>messy</quote></cit></sense></entry>",
    };
    const std::vector<std::string> paths {
        "/entry/form/orth",         // find
        "/entry/sense/cit/quote",   // don't find
    };

    CTQ::write(input_filename, output_filename, paths, 500);

    SECTION("C++") {
        CTQ::Reader reader(output_filename);

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
                auto find_orth  = reader.find(keys[i], false, 0, 0, 1);
                auto find_quote = reader.find(keys[i], false, 0, 0, 2);

                REQUIRE(find_orth.size() == 1);
                REQUIRE(find_quote.size() == 0);
            }
        }

        // range for "pleasant (fragrance)" and "plump"
        {
            auto find1 = reader.find("p", false, 0, 1);
            auto find2 = reader.find("p", false, 1, 1);

            REQUIRE(find1.size() == 1);
            REQUIRE(find2.size() == 1);
            REQUIRE(find1.begin()->first != find2.begin()->first);
        }

        // prefix
        {
            auto find = reader.find("p");

            REQUIRE(find.size() == 2);
        }

        // empty keyword
        {
            auto find = reader.find("");

            REQUIRE(find.size() > 0);
        }
    }

    SECTION("C") {
        ctq_ctx *ctx = ctq_create_reader(output_filename.c_str());

        for (int i = 0; i < keys.size(); ++i) {
            // default
            {
                ctq_find_ret *arr = ctq_find(ctx, keys[i].c_str(), false, 0, 0, 0);
                
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
            ctq_find_ret *arr = ctq_find(ctx, "p", false, 0, 0, 0);

            REQUIRE(arr != NULL);

            ctq_find_ret_free(arr);
        }

        // empty keyword
        {
            ctq_find_ret *arr = ctq_find(ctx, "", false, 0, 0, 0);

            REQUIRE(arr != NULL);

            ctq_find_ret_free(arr);
        }

        ctq_destroy_reader(ctx);
    }
}