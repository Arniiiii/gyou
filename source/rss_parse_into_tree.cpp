#include "gyou/rss_parse_into_tree.hpp"

#include <string>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
namespace gyou
{

    [[nodiscard]] boost::property_tree::ptree parse_rss_into_tree(
        std::string const& rss_feed)
    {
        boost::property_tree::ptree tree;
        std::istringstream istr(rss_feed);
        boost::property_tree::read_xml(istr, tree);
        return tree;
    }

}  // namespace gyou
