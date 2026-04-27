# VOLUMES DOCUMENTATION Version: 1.21.0.3


## Index


# Volumes


# Volume Components


These are the various possible components for this entity


| Name | Type | Default Value | Description |
| --- | --- | --- | --- |
| minecraft:fog |  |  | Displays the given fog whenever a player enters the volume. Each volume can only have one fog attached. |
| minecraft:on_actor_enter |  |  | Component that defines what happens when an actor enters the volume. Can contain multiple json objects. |
| minecraft:on_actor_leave |  |  | Component that defines what happens when an actor leaves the volume. |


> **minecraft:fog** 子参数


| Name | Type | Default Value | Description |
| --- | --- | --- | --- |
| fog_identifier | String |  | The identifier of a fog definition. Note that you will not receive any feedback if the definition does not exist. |
| priority | Integer | INT_MAX | The priority for this fog definition setting. Smaller numbers have higher priority. Fogs with equal priority will be combined together. |


> **minecraft:on_actor_enter** 子参数


| Name | Type | Default Value | Description |
| --- | --- | --- | --- |
| on_enter | Array |  | Required array that contains all the triggers.condition    Molang expression to test against the actor. The given event will be triggered if the expression evaluates to true.    event    Name of the event to run.    target    One of "self" or "other". Self means the event is attached to the volume. Other means the event is attached to the actor. |


> **minecraft:on_actor_leave** 子参数


| Name | Type | Default Value | Description |
| --- | --- | --- | --- |
| on_leave | Array |  | Required array that contains all the triggers.condition    Molang expression to test against the actor. The given event will be triggered if the expression evaluates to true.    event    Name of the event to run.    target    One of "self" or "other". Self means the event is attached to the volume. Other means the event is attached to the actor. |


# Volume Definition Properties


The properties of a minecraft:volume entity. All components are optional.


| Name | Type | Default Value | Description |
| --- | --- | --- | --- |
| format_version | String |  | Specifies the version of the game this entity was made in. Minimum supported version is 1.17.0. Current supported version is 1.21.0. |


## Example


Example


```json
{

  "format_version": 1.17.0,

  "minecraft:volume": {

    "description": {

      "identifier": "your_custom_namespace:sample_volume"

    },

    "components": {

      "minecraft:fog": {

        "fog_identifier": "minecraft:fog_savanna",

        "priority": 1

      }

    }

  }

}
```


# Volume Description Properties


The description contains a single 'identifier' string


| Name | Type | Default Value | Description |
| --- | --- | --- | --- |
| identifier | String |  | The unique identifier for this volume. It must be of the form 'namespace:name', where namespace cannot be 'minecraft'. |
