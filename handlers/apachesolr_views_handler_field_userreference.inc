<?php
/**
 * @file
 * provides a field handler for the snippet coming from the ApacheSolr search engine
 */
class apachesolr_views_handler_field_userreference extends views_handler_field {
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
    
    $names = array();
    foreach ($values as $uid) {
      $names[] = db_result(db_query('SELECT name FROM {users} WHERE uid = %d', $uid));
    }
    return implode(', ', $names);
  }
}
