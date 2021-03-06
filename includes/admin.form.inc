<?php

/**
 * @file
 * The admin form for the Islandora conditional access rights module.
 *
 *
 *  Copyright 2017 Leiden University Library
 *
 *  This file is part of islandora_conditional_access_rights.
 *
 *  islandora_conditional_access_rights is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/**
 * Implementation of a form. 
 *
 * @see drupal_get_form
 */

function islandora_conditional_access_rights_admin_form(array $form, array &$form_state) {
  module_load_include('inc', 'islandora_conditional_access_rights', 'includes/access_rights_settings');

  $form = array();

  if (isset($form_state['input']['config_location']) && strlen($form_state['input']['config_location']) > 0) {
    $config_location = $form_state['input']['config_location'];
  }
  else {
    $config_location = variable_get('islandora_conditional_access_rights_config_location');
  }
  $tn_url = variable_get('islandora_conditional_access_rights_tn_url');
  $defaults = islandora_conditional_access_rights_default_settings();

  $form['config'] = array(
    '#type' => 'fieldset',
    '#title' => t('Configuration'),
  );
  $form['config']['location'] = array(
    '#type' => 'textfield',
    '#title' => t('Configuration location'),
    '#description' => t('An absolute file path or URL to the configuration file.'),
    '#default_value' => $config_location,
  );
  $form['config']['tn_url'] = array(
    '#type' => 'textfield',
    '#title' => t('Access restricted thumbnail URL'),
    '#description' => t('A URL (absolute or relative to the server) of a thumbnail to use when the current item is not accessible by the user.'),
    '#default_value' => $tn_url,
  );
  $form['config']['defaults'] = array(
    '#type' => 'fieldset',
    '#title' => t('Defaults'),
  );
  foreach ($defaults as $defaultkey => $defaultsetting) {
    $default = variable_get('islandora_conditional_access_rights_default_' . $defaultkey, '');
    if ($defaultsetting['type'] === 'dsids') {
      if (isset($default) && is_array($default)) {
        $default = implode(',', $default);
      }
    }
    elseif ($defaultsetting['type'] === 'boolean') {
      if ($default !== '') {
        $default = ($default?'TRUE':'FALSE');
      }
    }
    $form['config']['defaults'][$defaultkey] = array(
      '#type' => 'textfield',
      '#title' => $defaultsetting['title'],
      '#description' => isset($defaultsetting['description'])?$defaultsetting['description']:'',
      '#default_value' => $default,
    );
  }
  $form['form_submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save configuration'),
  );

  return $form;
}

function islandora_conditional_access_rights_admin_form_validate(array $form, array &$form_state) {
  $newloc = $form_state['values']['location'];

  if (strlen($newloc) > 0) {
    module_load_include('inc', 'islandora_conditional_access_rights', 'includes/config');
    $error = '';
    if (islandora_conditional_access_rights_valid_config_file($newloc, $error) === FALSE) {
      form_set_error('location', $error);
    }
  }
  $defaults = islandora_conditional_access_rights_default_settings();
  foreach ($defaults as $defaultkey => $defaultsetting) {
    $value = $form_state['values'][$defaultkey];
    if (empty($value)) {
      // empty defaults are not an error.
    }
    elseif ($defaultsetting['type'] === 'dsids') {
      $dsids = explode(',', $value);
      foreach ($dsids as $dsid) {
        if (preg_match('/^[A-Z][A-Z0-9]+$/', $dsid) !== 1) {
          form_set_error($defaultkey, t("Datastream ID '%dsid' is not correct", array("%dsid" => $dsid)));
        }
      }
    }
    elseif ($defaultsetting['type'] === 'url') {
      if (filter_var($value, FILTER_VALIDATE_URL) === FALSE) {
        form_set_error($defaultkey, t("%title should be a URL", array("%title" => $defaultsetting['title'])));
      }
    }
    elseif ($defaultsetting['type'] === 'boolean') {
      if ($value !== 'TRUE' && $value !== 'FALSE') {
        form_set_error($defaultkey, t("%title should be a TRUE or FALSE (or empty)", array("%title" => $defaultsetting['title'])));
      }
    }
  }
}

function islandora_conditional_access_rights_admin_form_submit(array $form, array &$form_state) {
  module_load_include('inc', 'islandora_conditional_access_rights', 'includes/config');
  module_load_include('inc', 'islandora_conditional_access_rights', 'includes/access_rights_settings');

  $oldconfiglocation = variable_get('islandora_conditional_access_rights_config_location');
  variable_set('islandora_conditional_access_rights_config_location', $form_state['values']['location']);
  variable_set('islandora_conditional_access_rights_tn_url', $form_state['values']['tn_url']);
  if ($oldconfiglocation) {
    islandora_conditional_access_rights_refresh_configuration($oldconfiglocation);
  }
  $defaults = islandora_conditional_access_rights_default_settings();
  foreach ($defaults as $defaultkey => $defaultsetting) {
    if ($defaultsetting['type'] === 'dsids') {
      if ($form_state['values'][$defaultkey] === '') {
        variable_del('islandora_conditional_access_rights_default_' . $defaultkey);
      }
      else {
        variable_set('islandora_conditional_access_rights_default_' . $defaultkey, explode(',', $form_state['values'][$defaultkey]));
      }
    }
    elseif ($defaultsetting['type'] === 'boolean') {
      if ($form_state['values'][$defaultkey] === '') {
        variable_del('islandora_conditional_access_rights_default_' . $defaultkey);
      }
      else {
        variable_set('islandora_conditional_access_rights_default_' . $defaultkey, ($form_state['values'][$defaultkey] === 'TRUE')?TRUE:FALSE);
      }
    }
    else {
      variable_set('islandora_conditional_access_rights_default_' . $defaultkey, $form_state['values'][$defaultkey]);
    }
  }

  drupal_set_message(t('The settings have been saved!'));
}
