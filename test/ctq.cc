#include <string>

#include "catch2/catch_test_macros.hpp"
#include "ctq_writer.hh"
#include "ctq_reader.hh"

#include <vector>

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

    CTQ::write(input_filename, output_filename, paths);

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


        ++i;
    }

    // range for "pleasant (fragrance)" and "plump"
    {
        auto find1 = reader.find("p", false, 0, 1);
        auto find2 = reader.find("p", false, 1, 1);

        REQUIRE(find1.size() == 1);
        REQUIRE(find2.size() == 1);
        REQUIRE(find1.begin()->first != find2.begin()->first);
    }
}