<?php
/**
 * @file
 * Provides a handler filter for TID
 */

/**
 * Class for filtering by taxonomy ID.
 */
class apachesolr_views_handler_filter_tid extends views_handler_filter_term_node_tid {

  function query() {

    if (!empty($this->value)) {

      $not = ($this->operator == 'not');
      $op = ($this->operator == 'and') ? ' AND ' : ' OR ';

      $filter = "(" . join($op, $this->value) . ")";

      $this->query->add_filter($this->real_field, $filter, $not);

    }

  }

  function value_form(&$form, &$form_state) {
    parent::value_form($form, $form_state);
    $form['value']['#pre_render'] = array('apachesolr_views_handler_filter_cleanup');
    $form['value']['#vid'] = 'im_vid_' . $this->options['vid'];
  }

  function exposed_validate(&$form, &$form_state) {

    parent::exposed_validate($form, $form_state);

    $identifier = $this->options['expose']['identifier'];
    $form_element = $form[$identifier];

    if (empty($this->options['expose']['optional']) && empty($this->value)) {
      form_error($form_element, t('Field @name cannot be left blank.', array('@name' => $this->options['expose']['label'])));
    }

    if (form_get_error($form_element)) {
      $this->view->executed = TRUE;
    }

  }

}
