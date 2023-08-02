# Armor Blasting
<p align="center">
   <img src="https://github.com/LDiazN/ArmorBlasting/assets/41093870/ddd9b1ce-a067-4fad-af30-46312bd93ec7" alt="Shotgun Preview"/>
</p>

A simple Unreal Engine 4 sample project that showcases dynamic damage over meshes. Shoot at the armored robot to blast its armor and see through it. I also provide three different shooting styles to test how this effect behaves with some of the most common weapon types in FPS games: a marksman rifle, a full auto rifle, and the mighty shotgun!

The project starts from the standard Unreal Engine First Person Shooter starter pack. I created the robot model in Blender because I wanted to go with a low-poly aesthetic and I needed a model with armor that would match the model and meet some specific properties that I will explain below. The project code is almost entirely implemented with C++, while also using blueprints when more fit for the task. 

## Click to watch full demo in YouTube

<p align="center">
<a href="https://www.youtube.com/watch?v=gGf-r-gn8gM" target="_blank" align="center">
 <img src="https://i.ytimg.com/vi/FS4jb0s5NAk/0.jpg" alt="Watch the gameplay video in YouTube" border="10" />
</a>
</p>

# Samples
In the following section I will showcase and briefly explain some features of this project. All shooting modes are implemented using a simple function 
provided by the `BlastableComponent` Scene Component: `void Blast(FVector Location, float ImpactRadius)`. With this simple function you can implement many ways to 
damage the character. The `Location` is a position in world space where the center of the damage occured, and `ImpactRadius` is the damage radius. This means that damage
is represented as a sphere intersecting some parts of the character's armor. The only affected parts are static meshes tagged with `BlastableMesh`.

## Shotgun
The shotgun shooting mode will trace many rays randomly into a cone starting from the weapon muzzle, and the impact radius will be bigger when the shot ray is near the center of the cone.

## Marksman Rifle
## Full Auto Rifle
## Damage Maps
### Permanent Damage
### Temporal Damage
# Implementation Details
# Mesh Unwrapping 
