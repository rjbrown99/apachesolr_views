<?php

/**
 * generic filter for Apache Solr
 */
class apachesolr_views_handler_filter_numeric extends views_handler_filter_numeric {

  function construct() {
    parent::construct();
    $this->definition['float'] = TRUE;
  }

  /**
   * override query().
   */
  public function query() {
    // clean number - must be a better way - tie in to format_number, add
    // validation to parent handler
    foreach ($this->value as $field => $value) {
      if (isset($field) && !empty($value)) {
        $this->value[$field] = preg_replace('/[^0-9.]/', '', $value);
      }
    }

    switch ($this->operator) {
      case '<':
        // Ugly but for now the only way to do it, wait for solr 4
        $this->query->add_filter($this->real_field, $this->value['value'], TRUE);
      case '<=':
        $this->query->add_filter($this->real_field, '[* TO ' . $this->value['value'] . ']');
        break;
      case '=':
        $this->query->add_filter($this->real_field, $this->value['value']);
        break;
      case '!=':
        $this->query->add_filter($this->real_field, $this->value['value'], TRUE);
        break;
      case '>':
        $this->query->add_filter($this->real_field, $this->value['value'], TRUE);
      case '>=':
        $this->query->add_filter($this->real_field, '[' . $this->value['value'] . ' TO *]');
        break;
      case 'between':
      case 'not between':
        $min = empty($this->value['min']) ? '*' : $this->value['min'];
        $max = empty($this->value['max']) ? '*' : $this->value['max'];
        $exclude = ($this->operator == 'between') ? FALSE : TRUE;
        if (!($min == '*' && $max == '*')) {
          $this->query->add_filter($this->real_field, "[$min TO $max]", $exclude);
        }
        break;
    }
  }

  /**
   * Validate the exposed filter input form.
   * Will not be required if added to views_handler_filter_numeric see:
   * http://drupal.org/node/1321640
   */
  function exposed_validate($form, &$form_state) {
    parent::exposed_validate($form, $form_state);

    $identifier = $this->options['expose']['identifier'];
    $form_element = $form[$identifier];

    // Exposed form validation - views uses $form_state['input'] rather
    // than $form_state['values']
    if ($this->options['exposed'] && isset($form_state['input'][$identifier])) {
      foreach ($form_state['input'][$identifier] as $key => $value) {
        // if required
        if (!$this->options['expose']['optional'] && empty($value)) {
          // Use $key in error text because there could be 'min' and 'max' fields
          form_error($form_element[$key], t('Field @name @key is required.',
              array('@name' => $this->options['expose']['label'], '@key' => $key)));
        }
        // check if numeric
        if (!empty($value) && !is_numeric($value))
          form_error($form_element[$key], t('Field @name @key must be a number.',
              array('@name' => $this->options['expose']['label'], '@key' => $key)));
      }
    }
  }

}
