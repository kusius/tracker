#pragma once
#include <vector>

#include "datastore.h"
#include "gumbo.h"

typedef std::vector<GumboNode> NodeV;

GumboNode FindFirst(GumboNode* node,
                    GumboTag tag,
                    const char* attrib_name,
                    const char* attrib_value) {
  GumboNode result;
  GumboAttribute* attr_class;
  GumboNode empty_node;
  empty_node.type = GUMBO_NODE_TEMPLATE;

  if (node->type != GUMBO_NODE_ELEMENT)
    return (empty_node);

  // Found *a* table
  if (node->v.element.tag == tag) {
    if (attrib_name != nullptr) {
      attr_class =
          gumbo_get_attribute(&node->v.element.attributes, attrib_name);
      if (strcmp(attrib_value, attr_class->value) == 0) {
        // Found the results table
        return (*node);
      }
    } else
      return (*node);
  }

  GumboVector* children = &node->v.element.children;
  for (unsigned int i = 0; i < children->length; ++i) {
    result = FindFirst(static_cast<GumboNode*>(children->data[i]), tag,
                       attrib_name, attrib_value);

    if (result.type != GUMBO_NODE_TEMPLATE)
      return (result);
  }
  return (empty_node);
}

std::vector<GumboNode> FindAll(GumboNode* node,
                               GumboTag tag,
                               const char* attrib_name,
                               const char* attrib_value) {
  NodeV results;

  GumboAttribute* attr_class;
  GumboNode empty_node;
  empty_node.type = GUMBO_NODE_TEMPLATE;

  if (node->type != GUMBO_NODE_ELEMENT)
    return (NodeV());

  // Match tag
  if (node->v.element.tag == tag) {
    // If given, match an attribute
    if (attrib_name != nullptr) {
      attr_class =
          gumbo_get_attribute(&node->v.element.attributes, attrib_name);
      if (strcmp(attrib_value, attr_class->value) == 0) {
        results.push_back(*node);
        return (results);
      }
    } else {
      results.push_back(*node);
      return (results);
    }
  }

  GumboVector* children = &node->v.element.children;
  std::vector<GumboNode> children_results;
  for (unsigned int i = 0; i < children->length; ++i) {
    children_results = FindAll(static_cast<GumboNode*>(children->data[i]), tag,
                               attrib_name, attrib_value);

    if (!children_results.empty())
      results.insert(results.end(), children_results.begin(),
                     children_results.end());
  }

  return (results);
}

size_t GetElementLength(GumboElement* element) {
  return (element->end_pos.offset - element->start_pos.offset);
}

std::string GetInnerHTML(GumboElement* element) {
  return (std::string(element->original_tag.data, GetElementLength(element)));
}

std::string GetText(GumboNode* node) {
  GumboVector* head_children = &node->v.element.children;
  for (unsigned int i = 0; i < head_children->length; ++i) {
    GumboNode* child = static_cast<GumboNode*>(head_children->data[i]);
    if (child->type == GUMBO_NODE_TEXT || GUMBO_NODE_WHITESPACE) {
      return (std::string(child->v.text.text));
    }
  }
  return ("");
}

// Strip all characters except numbers and '.'
std::string KeepNumber(std::string source) {
  std::string result;
  const char* start = source.c_str();
  size_t length = source.length();
  for (int i = 0; i < length; i++) {
    char c = start[i];
    if ((c >= 48 && c <= 57) || c == 46)
      result += c;
  }
  return (result);
}

// Strip all characters except letters a-z A-Z
std::string KeepText(std::string source) {
  std::string result;
  const char* start = source.c_str();
  size_t length = source.length();
  for (int i = 0; i < length; i++) {
    char c = start[i];
    if ((c >= 65 && c <= 90) || (c >= 97 && c <= 122))
      result += c;
  }
  return (result);
}

// Returns:
// True if item exists (updated)
// False if item does not exist (new)
// False and an empty structure if item not found
bool ParseTTCPriceCheck(std::string HTML, PriceCheck* item) {
  *item = {};
  GumboOutput* output = gumbo_parse(HTML.c_str());

  // Get html TABLE containing the results
  GumboNode results_table = FindFirst(output->root, GUMBO_TAG_TABLE, "class",
                                      "trade-list-table max-width");
  // Get all ROWS of that table
  NodeV all_rows = FindAll(&results_table, GUMBO_TAG_TR, nullptr, nullptr);
  if (all_rows.size() > 0) {
    // Second ROW contains the first item match
    GumboNode price_check_node = all_rows[1];

    // Get all table COLUMNS
    NodeV all_data = FindAll(&price_check_node, GUMBO_TAG_TD, nullptr, nullptr);

    if (all_data.size() > 0) {
      // First column has item data and third has suggested prices
      GumboNode item_data = all_data[0];
      GumboNode suggested_price = all_data[2];
      // Parse item name
      GumboNode item_name_div =
          FindFirst(&all_data[0], GUMBO_TAG_DIV, nullptr, nullptr);
      item->name = KeepText(GetText(&item_name_div));

      // Parse suggested prices to double
      NodeV suggestions =
          FindAll(&suggested_price, GUMBO_TAG_SPAN, nullptr, nullptr);
      if (suggestions.size() > 0) {
        std::string min_suggest = GetText(&suggestions[0]);
        std::string max_suggest = GetText(&suggestions[1]);

        min_suggest = KeepNumber(min_suggest);
        max_suggest = KeepNumber(max_suggest);

        item->min_suggest = ::atof(min_suggest.c_str());
        item->max_suggest = ::atof(max_suggest.c_str());

        // Save the inner HTML of the item info
        item->inner_html = GetInnerHTML(&all_data[0].v.element);

        // Find image src attribute path, append domain name and save
        GumboAttribute* attr_src;
        GumboNode img = FindFirst(&item_data, GUMBO_TAG_IMG, nullptr, nullptr);

        attr_src = gumbo_get_attribute(&img.v.element.attributes, "src");
        item->img_src = std::string("https://eu.tamrieltradecentre.com") +
                        std::string(attr_src->value);
        item->is_watched = true;
        return DataStore::AddItem(*item);
      }
    }
  }
  return false;
}