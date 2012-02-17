<?php
// $Id: apachesolr_views_handler_field_nodereference.inc 1896 2012-02-17 22:08:42Z rbrown $

/**
 * @file
 * provides a field handler for the snippet coming from the ApacheSolr search engine
 */
class apachesolr_views_handler_field_nodereference extends views_handler_field {
  function query() {
    $this->query->add_field($this->real_field);
  }

  function render($doc) {
    if (is_array($doc->{$this->real_field})) {
      $values = $doc->{$this->real_field};
    }
    else {
      $values = array($doc->{$this->real_field});
    }
    
    $titles = array();
    foreach ($values as $nid) {
      $titles[] = db_result(db_query('SELECT title FROM {node} WHERE nid = %d', $nid));
    }
    return implode(', ', $titles);
  }
}
