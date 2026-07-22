#ifndef INCLUDE_GYOU_RSS_PARSE_INTO_TREE_HPP_
#define INCLUDE_GYOU_RSS_PARSE_INTO_TREE_HPP_

#include <string>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
namespace gyou
{

    [[nodiscard]] inline boost::property_tree::ptree parse_rss_into_tree(
        std::string const& rss_feed)
    {
        boost::property_tree::ptree tree;
        std::istringstream istr(rss_feed);
        boost::property_tree::read_xml(istr, tree);
        return tree;
    }

}  // namespace gyou

#endif  // INCLUDE_GYOU_RSS_PARSE_INTO_TREE_HPP_
