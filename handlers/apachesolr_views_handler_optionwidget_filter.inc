<?php
class apachesolr_views_handler_optionwidget_filter extends views_handler_filter_many_to_one {
  function query() {
    //operators: or, and, not
    
    $exclude = FALSE;
    if ($this->operator == 'not') $exclude = TRUE;
        
    // sometimes value is a string and other times an array - probably caused by bug
    // since we are dealing with select lists it should probably be array
    if (is_string($this->value)) {
      $this->value = array($this->value);
    };
    
    // values with spaces have to be wrapped in quotes
    // why doesn't this happen by default elsewhere?
    foreach ($this->value as &$value) {
      if (strstr($value, ' ')) $value = '"' . $value . '"';
    }
    
    if ($this->operator == 'or' && count($this->value)>1) {
      $sub_query = apachesolr_drupal_query('');
      foreach ($this->value as $value) {
        $sub_query->add_filter($this->real_field, $value);
      }
      $this->query->add_subquery($sub_query, 'OR');
    } else {
      // 'and' & 'not' operators
      foreach ($this->value as $value) {
        $this->query->add_filter($this->real_field, $value, $exclude);
      }
    }
  }

  function get_value_options() {
    if (isset($this->value_options)) {
      return;
    }
    $this->value_options = content_allowed_values(content_fields($this->definition['cck_field']['field_name']));
  }
}
