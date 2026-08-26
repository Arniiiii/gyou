#ifndef INCLUDE_GYOU_RSS_PARSE_INTO_TREE_HPP_
#define INCLUDE_GYOU_RSS_PARSE_INTO_TREE_HPP_

#include <string>

#include <boost/property_tree/ptree.hpp>
namespace gyou
{

    [[nodiscard]] boost::property_tree::ptree parse_rss_into_tree(
        std::string const& rss_feed);

}  // namespace gyou

#endif  // INCLUDE_GYOU_RSS_PARSE_INTO_TREE_HPP_
