{
  "entities": [
    {
      "type": "Entity",
      "id": "MainCameraId",
      "name": "MainCamera",
      "transform": {
        "position": {
          "x": 0,
          "y": 0,
          "z": 0
        },
        "rotation": {
          "x": 0,
          "y": 0,
          "z": 0,
          "w": 1
        },
        "scale": {
          "x": 1,
          "y": 1,
          "z": 1
        }
      },
      "components": [
        {
          "type": "CameraComponent",
          "isActive": true,
          "defaultsToMain": true,
          "perspective": {
            "aspect": 1.77,
            "fov": 90,
            "zFar": 10000,
            "zNear": 0.1
          }
        }
      ]
    }
  ],
  "name": "main",
  "type": "Scene"
}
