<?php

/**
 * Load an item
 * 
 * @param $type
 *   The type of item to load
 * @param $item_id
 *   The id of the item to load
 *
 * @return
 *   The loaded item, or if not found FALSE.
 */
function kw_aliases_item_load($type, $item_id) {
  return kw_aliases_type_call_callback($type, 'item load',
    array('%item_id'), array('%item_id' => $item_id)
  );
}

/**
 * Create an aliasable item.

 * @param $type
 *   The type of item that should be created.
 * @param $defaults
 *   The properties of the item that should be set on initial creation.
 * @param $required
 *   The properties of the item that are required. These properties will 
 *   overwrite the defaults.
 *
 * @return
 *   The created item.
 */
function kw_aliases_item_create($type, $defaults = array(), $required = array()) {
  return kw_aliases_type_call_callback($type, 'item create',
    array('%defaults', '%required'), array('%defaults' => $defaults, '%required' => $required)
  );
}

/**
 * Update a aliased item.

 * @param $type
 *   The type of item that should be updated.
 * @param $item
 *   The actual item that should be updated.
 * @param $required
 *   The properties of the item that are required. These properties will 
 *   overwrite exisiting properties on the item.
 *
 * @return
 *   The created item.
 */
function kw_aliases_item_update($type, $item, $required = array()) {
  return kw_aliases_type_call_callback($type, 'item update', 
    array('%item', '%required'), array('%item' => $item, '%required' => $required)
  );
}

/**
 * Get the ID of an aliased item.
 *
 * @param $type
 *   The type of item of which the ID should be returned.
 * @param $item
 *   The actual item of which the ID should be returned.
 *
 * @return
 *   The ID of the item.
 */
function kw_aliases_item_get_item_id($type, $item) {
  return kw_aliases_type_call_callback($type, 'item extract id', 
    array('%item'), array('%item' => $item)
  );
}

/**
 * Change an alias to point to an item
 *
 * @param $type
 *   The type of item to which the alias should point.
 * @param $alias
 *   The alias that should point to the item.
 * @param $item
 *   The item to which the alias should point.
 *
 * @return
 *   TRUE if successfull. FALSE on error.
 */
function kw_aliases_alias_update($type, $alias, $item) {
  if (!($item_id = kw_aliases_item_get_item_id($type, $item))) {
    return FALSE;
  }
  return kw_aliases_alias_set_item_id($type, $alias, $item_id);
}

/**
 * Get the id of an aliased item.
 *
 * @param $type
 *   The type of alias to get.
 * @param $alias
 *   The alias to set
 *
 * @return
 *   The id of the item, or if not found FALSE.
 */
function kw_aliases_alias_get_item_id($type, $alias) {
  $result = db_select('kw_aliases', 'ka')
    ->fields('ka', array('item_id'))
    ->condition('type', $type)->condition('alias', $alias)
    ->execute()
    ->fetchField();
  return $result ? $result : FALSE;
}

/**
 * Set the id of an aliased item.
 *
 * @param $type
 *   The type of alias to set.
 * @param $alias
 *   The alias to set
 * @param $item_id
 *   The id to set
 */
function kw_aliases_alias_set_item_id($type, $alias, $item_id) {
  db_merge('kw_aliases')
    ->key(array('type' => $type, 'alias' => $alias))
    ->fields(array('item_id' => $item_id))
    ->execute();
  return TRUE;
}

/**
 * Enable deletion prevention on an item.
 *
 * @param $type
 *   The type of item.
 * @param $item
 *   The item.
 */
function kw_aliases_item_prevent_deletion($type, $item) {
  db_merge('kw_aliases_deletion_prevention')
    ->key(array('type' => $type, 'item_id' => kw_aliases_item_get_item_id($type, $item)))
    ->execute();
}

/**
 * Disable deletion prevention on an item.
 *
 * @param $type
 *   The type of item.
 * @param $item
 *   The item.
 */
function kw_aliases_item_allow_deletion($type, $item) {
  $item_id = kw_aliases_item_get_item_id($type, $item);
  db_delete('kw_aliases_deletion_prevention')
    ->condition('type', $type)->condition('item_id', $item_id)
    ->execute();
}

/**
 * Call a callback for an alias type.
 *
 * @param $type
 *   The alias type.
 * @param $callback_type
 *   The calback that should be executed.
 * @param $args
 *   The default arguments for the callback. Can be overriden by the 
 *   $callback_type . ' arguments' key of a type info array.
 * @param $replace
 *   Placeholders in $args that get replaced by useful values. This makes 
 *   actually feasible to override the default arguments.
 * 
 * @return
 *   The return value of the callback. FALSE if the callback can't be called.
 */
function kw_aliases_type_call_callback($type, $callback_type, $args = array(), $replace = array()) {
  $info = kw_aliases_type_info($type);

  $callback_key = $callback_type . ' callback';
  if (!$info || !isset($info[$callback_key])) {
    return FALSE;
  }

  // load required files
  module_load_include('kw_aliases.inc', $info['module']);
  if (isset($info['file'])) {
    $path = isset($info['file path']) ? $info['file path'] : drupal_get_path('module', $info['module']);
    $filepath = $path . DIRECTORY_SEPARATOR . $info['file'];
    if (is_file($filepath)) {
      require_once $filepath;
    } 
  }

  if (!function_exists($info[$callback_key])) {
    return FALSE;
  }


  $arguments_key = $callback_type . ' arguments';
  if (isset($info[$arguments_key])) {
    $args = $info[$arguments_key];
  }

  if (!empty($replace)) {
    $args = _kw_aliases_replace_arguments($args, $replace);
  }

  return call_user_func_array($info[$callback_key], $args);
}

function _kw_aliases_replace_arguments($args, $replace = array()) {
  $result = array();
  foreach ($args as $key => $arg) {
    if (is_array($arg)) {
      $result[$key] = _kw_aliases_replace_arguments($arg, $replace);
    } elseif (is_scalar($arg) && array_key_exists($arg, $replace)) {
      $result[$key] = $replace[$arg];
    } else {
      $result[$key] = $arg;
    }
  }
  return $result;
}