<?php

/**
 * @file
 * Retrieve the metadata value from an object, using caching.
 */

function islandora_conditional_access_rights_get_metadata_values($obj, $config, $neededmdkey, $rebuildcache = FALSE) {
  $id2mdvalue = &drupal_static(__FUNCTION__, array());
  $id = $obj->id;
  $lastchangedate = (string)$obj->lastModifiedDate;
  if ($rebuildcache) {
    unset($id2mdvalue[$id]);
  }
  if (!isset($id2mdvalue[$id][$neededmdkey])) {
    $cachename = $id;
    $cachebin = 'cache_islandora_conditional_access_rights_md_value';
    if (!$rebuildcache) {
      if ($cache = cache_get($cachename, $cachebin)) {
        $found = $cache->data;
        if ($found['_date'] === $lastchangedate) {
          $id2mdvalue[$id] = $found;
        }
      }
    }
    if (!isset($id2mdvalue[$id])) {
      $id2mdvalue[$id] = array('_date' => $lastchangedate);
      $mdsettings = $config['metadata'];
      foreach ($mdsettings as $mdkey => $mdvalues) {
        if (isset($mdvalues['solrfield'])) {
          $solrvalues = _islandora_conditional_access_rights_get_solr_values($obj, $mdvalues['solrfield']);
          $id2mdvalue[$id][$mdkey] = $solrvalues;
        }
        else {
          $dsid = $mdvalues['datastream'];
          $id2mdvalue[$id][$mdkey] = NULL;
          if (isset($obj[$dsid])) {
            $xpaths = $mdvalues['xpath'];
            $remove = array();

            $content = $obj[$dsid]->content;
            $domdoc = new DOMDocument();
            if ($domdoc->loadXML($content)) {
              $id2mdvalue[$id][$mdkey] = ''; // subtle change to indicate that the DS XML exists, but does not have the metadata value
              $domxpath = new DOMXPath($domdoc);
              if (isset($mdvalues['namespace'])) {
                foreach ($mdvalues['namespace'] as $ns) {
                  list($prefix, $uri) = explode(' ', $ns, 2);
                  $domxpath->registerNamespace($prefix, $uri);
                }
              }
              $xpath = $mdvalues['xpath'];
              $domnodelist = $domxpath->query($xpath);
              if ($domnodelist && $domnodelist->length > 0) {
                foreach ($domnodelist as $domnode) {
                  $text = $domnode->textContent;
                  if (isset($text) && strlen($text) > 0) {
                    if (!isset($id2mdvalue[$id][$mdkey])) {
                      $id2mdvalue[$id][$mdkey] = array();
                    }
                    $id2mdvalue[$id][$mdkey][] = $text;
                  }
                }
              }
            }
          }
        }
      }
      cache_set($cachename, $id2mdvalue[$id], $cachebin);
    }
  }
  return $id2mdvalue[$id][$neededmdkey];
}

function _islandora_conditional_access_rights_get_solr_values($obj, $solrfield) {
  static $qp = NULL;
  static $qpc = 0;
  static $solrresults = array();

  if (isset($solrresults[$obj->id][$solrfield])) {
    return $solrresults[$obj->id][$solrfield];
  }

  if (is_null($qp) || $qpc > 10) {
    // Get a fresh copy the first time and after each 10 queries.
    // Apparently there are limitations on the amount of queries it can handle.
    $qp = new IslandoraSolrQueryProcessor();
    $qpc = 0;
  }
  $qpc++;

  $escapedid = islandora_solr_lesser_escape($obj->id);
  $search = "PID:$escapedid";

  $qp->buildAndExecuteQuery($search, NULL, FALSE);
  $r = $qp->islandoraSolrResult;
  $num_found = $r['response']['numFound'];
  if ($num_found == 1) {
    $solrresults[$obj->id] = $r['response']['objects'][0]['solr_doc'];
    if (isset($solrresults[$obj->id][$solrfield])) {
      $value = $solrresults[$obj->id][$solrfield];

      if (!is_array($value)) {
        $value = array($value);
      }
      return $value;
    }
  }
  return NULL;
}
