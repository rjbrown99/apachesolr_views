<?php
/**
 * @file
 * Provides a handler filter for userreference fields
 */

/**
 * Class for filtering by userreference uid.
 */
class apachesolr_views_handler_filter_userreference extends content_handler_filter_many_to_one {

  public function query() {

    $not = $this->operator == 'not in';
    $uids = (array)$this->value;

    if (empty($uids)) {
      if (!$not) {
        $this->query->add_filter($this->real_field, apachesolr_views_query::escape_term('-1'));
      }
    }
    else {
      foreach ($uids as $uid) {
        $filter_string .= " OR $uid";
      }

      // Construct a small subquery.
      $filter_string = drupal_substr($filter_string, 4);
      $filter_string = "($filter_string)";

      // Add it to the query.
      $this->query->add_filter($this->real_field, $filter_string, $not);
    }
  }

  function has_extra_options() {
    return TRUE;
  }

  function option_definition() {
    $options = parent::option_definition();
    $options['type'] = array('default' => 'textfield');
    return $options;
  }

  function extra_options_form(&$form, &$form_state) {
    $form['markup_start'] = array(
      '#value' => '<div class="views-left-40">',
    );
    $form['type'] = array(
      '#type' => 'radios',
      '#title' => t('Selection type'),
      '#options' => array('select' => t('Dropdown'), 'textfield' => t('Autocomplete')),
      '#default_value' => $this->options['type'],
    );
    $form['markup_end'] = array(
      '#value' => '</div>',
    );
  }

  /**
   * Get allowed values from hook_allowed_values(), if any, or from
   * content_allowed_values();
   */
  function allowed_values() {
    $options = userreference_allowed_values(content_fields($this->definition['cck_field']['field_name']));;
    return (array) $options;
  }

  function value_form(&$form, &$form_state) {
    $form['value'] = array();
 
    if ($this->options['type'] == 'select') {
      $this->get_value_options();
      $options = $this->value_options;
      $default_value = (array) $this->value;

      $which = 'all';
      if (!empty($form['operator'])) {
        $source = ($form['operator']['#type'] == 'radios') ? 'radio:options[operator]' : 'edit-options-operator';
      }
      if (!empty($form_state['exposed'])) {
        $identifier = $this->options['expose']['identifier'];

        if (empty($this->options['expose']['use_operator']) || empty($this->options['expose']['operator'])) {
          // Exposed and locked.
          $which = in_array($this->operator, $this->operator_values(1)) ? 'value' : 'none';
        }
        else {
          $source = 'edit-' . form_clean_id($this->options['expose']['operator']);
        }

        if (!empty($this->options['expose']['reduce'])) {
          $options = $this->reduce_value_options();

          if (empty($this->options['expose']['single']) && !empty($this->options['expose']['optional'])) {
            $default_value = array();
          }
        }

        if (!empty($this->options['expose']['single'])) {
          if (!empty($this->options['expose']['optional']) && (empty($default_value) || !empty($this->options['expose']['reduce']))) {
            $default_value = 'All';
          }
          elseif (empty($default_value)) {
            $keys = array_keys($options);
            $default_value = array_shift($keys);
          }
          else {
            $copy = $default_value;
            $default_value = array_shift($copy);
          }
        }
      }

      if ($which == 'all' || $which == 'value') {
        if ($this->value_form_type == 'checkboxes') {
          foreach ($options as $key => $option) {
            $options[$key] = check_plain($option);
          }
        }
        $form['value'] = array(
          '#type' => $this->value_form_type,
          '#title' => $this->value_title,
          '#options' => $options,
          '#default_value' => $default_value,
          // These are only valid for 'select' type, but do no harm to checkboxes.
          '#multiple' => TRUE,
          '#size' => count($options) > 8 ? 8 : count($options),
          '#pre_render' => array('apachesolr_views_handler_filter_cleanup'),
        );

        if (!empty($form_state['exposed']) && !isset($form_state['input'][$identifier])) {
          $form_state['input'][$identifier] = $default_value;
        }

        if ($which == 'all') {
          $process = array();
          if ($this->value_form_type == 'checkboxes' || $this->value_form_type == 'radios') {
            $process[] = "expand_$this->value_form_type";
            $form['value']['#prefix'] = '<div id="edit-options-value-wrapper">';
            $form['value']['#suffix'] = '</div>';
          }
          $process[] = 'views_process_dependency';
          $form['value'] += array(
            '#process' => $process,
            '#dependency' => array($source => $this->operator_values(1)),
          );
        }
      }

      if (empty($form_state['exposed'])) {
        $this->helper->options_form($form, $form_state);
      }
    }
    elseif ($this->options['type'] == 'textfield') {
      $default = '';
      if (is_numeric($this->value)) {
        $default = check_plain(db_result(db_query('SELECT name FROM {users} WHERE uid = %d', $this->value)));
        if ($this->options['expose']['show_uid']) {
          $default .= ' [uid:' . $this->value . ']';
        }
      }
      else {
        $default = $this->value;
      }

      $form['value'] = array(
        '#title' => $this->value_title,
        '#type' => 'textfield',
        '#default_value' => $default,
      );
      if ($this->options['expose']['show_uid']) {
        $form['value']['#autocomplete_path'] = 'userreference/autocomplete/'. $this->definition['cck_field']['field_name'];
      }
      else {
        $form['value']['#autocomplete_path'] = 'apachesolr-views/userreference/autocomplete/'. $this->definition['cck_field']['field_name'];
      }
    }
  }

  function value_validate(&$form, &$form_state) {
    // We only validate if they've chosen the text field style.
    if ($this->options['type'] != 'textfield') {
      return;
    }

    $uid = $this->userreference_validate($form, $form_state['values']['options']['value']);

    $form_state['values']['options']['value'] = $uid;
  }

  function exposed_validate(&$form, &$form_state) {
    if (empty($this->options['exposed'])) {
      return;
    }

    $identifier = $this->options['expose']['identifier'];

    // We only validate if they've chosen the text field style.
    if ($this->options['type'] != 'textfield') {

      if ($form_state['values'][$identifier] != 'All')  {
        $this->validated_exposed_input = (array) $form_state['values'][$identifier];
      }
      return;
    }

    if (empty($this->options['expose']['identifier'])) {
      return;
    }
    $uid = $this->userreference_validate($form, $form_state['values'][$identifier]);
    $this->validated_exposed_input = $uid;
    $form_state['values'][$identifier] = $uid;
  }

  function userreference_validate(&$form, $value) {
    $field = content_fields($this->definition['cck_field']['field_name']);
    if (!empty($value)) {
      preg_match('/^(?:\s*|(.*) )?\[\s*uid\s*:\s*(\d+)\s*\]$/', $value, $matches);
      if (!empty($matches)) {
        // Explicit [uid:n].
        list(, $title, $uid) = $matches;
        if (!empty($title) && ($n = user_load($uid)) && $title != $n->name) {
          form_error($form['value'], t('%name: title mismatch. Please check your selection.', array('%name' => t($field['widget']['label']))));
        }
      }
      else {
        // No explicit uid.
        $reference = _userreference_potential_references($field, $value, 'equals', NULL, 1);
        if (empty($reference)) {
          form_error($form['value'], t('%name: found no valid post with that title.', array('%name' => t($field['widget']['label']))));
        }
        else {
          $uid = key($reference);
        }
      }
    }

    return $uid;
  }

  function expose_options() {
    parent::expose_options();
    $this->options['expose']['show_uid'] = TRUE;
  }

  function expose_form_right(&$form, &$form_state) {
    parent::expose_form_right($form, $form_state);
    $form['expose']['show_uid'] = array(
      '#type' => 'checkbox',
      '#title' => t('Show uid in text field'),
      '#description' => t('If checked, the autocomplete field will contain the node id. This can yield more accurate results.'),
      '#default_value' => !empty($this->options['expose']['show_uid']),
    );
  }
}
