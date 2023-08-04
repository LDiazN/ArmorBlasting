# Armor Blasting 💥
<p align="center">
   <img src="https://github.com/LDiazN/ArmorBlasting/assets/41093870/ddd9b1ce-a067-4fad-af30-46312bd93ec7" alt="Shotgun Preview"/>
</p>

A simple Unreal Engine 4 sample project that showcases dynamic damage over meshes. Shoot at the armored robot to blast its armor and see through it. I also provide three different shooting styles to test how this effect behaves with some of the most common weapon types in FPS games: a marksman rifle, a full auto rifle, and the mighty shotgun!

The project starts from the standard Unreal Engine First Person Shooter starter pack. I created the robot model in Blender because I wanted to go with a low-poly aesthetic and I needed a model with armor that would match the model and meet some specific properties that I will explain below. The project code is almost entirely implemented with C++, while also using blueprints when more fit for the task. 

## Click to watch full demo in YouTube ⏯️

<p align="center">
<a href="https://www.youtube.com/watch?v=FS4jb0s5NAk" target="_blank" align="center">
 <img src="https://i.ytimg.com/vi/FS4jb0s5NAk/0.jpg" alt="Watch the gameplay video in YouTube" border="10" />
</a>
</p>

# Samples
In the following section I will showcase and briefly explain some features of this project. All shooting modes are implemented using a simple function 
provided by the `BlastableComponent` Scene Component: `void Blast(FVector Location, float ImpactRadius)`. With this simple function you can implement many ways to 
damage the character. The `Location` is a position in world space where the center of the damage occured, and `ImpactRadius` is the damage radius. This means that damage
is represented as a sphere intersecting some parts of the character's armor. The only affected parts are static meshes tagged with `BlastableMesh`.

Also, sparks are added at the impact point for aesthetic purposes!

The two images behind the robot character are used to display the damage maps used as input for the destruction material. 

## Shotgun
The shotgun shooting mode will trace many rays randomly into a cone starting from the weapon muzzle, and the impact radius will be bigger when the shot ray is near the center of the cone.
<p align="center">
   <img src="https://github.com/LDiazN/ArmorBlasting/assets/41093870/ddd9b1ce-a067-4fad-af30-46312bd93ec7" alt="Shotgun Preview"/>
</p>

## Marksman Rifle
The shooting mode is a line trace that starts at the weapon muzzle and goes in the direction of the center of the screen. The impact radius is fixed.
<p align="center">
   <img src="https://github.com/LDiazN/ArmorBlasting/assets/41093870/abae4cd1-8049-47e2-a782-cbcafa251a97" alt="Marksman Preview"/>
</p>

## Full Auto Rifle
This is the same as the marksman shot, but you can hold the shoot button to fire many shots per second.
<p align="center">
   <img src="https://github.com/LDiazN/ArmorBlasting/assets/41093870/d3c4b61b-d69a-408f-803c-4783c00c450f" alt="Full Auto Preview"/>
</p>

## Damage Maps
The blastable component stores damage over the mesh surface using two maps (implemented with render targets): a **Damage Map** and a **Temporal Damage Map**. These maps are then fed to the armor material as texture parameters, so that they can be sampled from the armor shader and implement any effect you like. In my case, I wanted to poke holes in the armor with shots while making the hole border glow to represent melted metal. The Damage Map is used for the first effect, while the Temporal Damage Map is used for the second.  

Note that the damage is sampled from each map using the armor texture coordinates.
### Permanent Damage

As its name implies, this texture/render target is used to store the damage dealt to the enemy's armor since the game started. It is a 1024x1024 texture that can be sampled using texture coordinates to get the damage state in each position (damaged or not damaged). Since the texture has multiple channels, you can modify the way the data is stored in the texture to store a different type of data in each channel, such as damage types or the damage position/impact normal. On `BeginPlay`, the `BlastableComponent` will access the armor mesh and set the texture parameter `RT_UnwrapDamage` of its material to this render target.

![Damage Map Example](https://github.com/LDiazN/ArmorBlasting/assets/41093870/c027363f-6eaa-4f3b-bee6-e7922b27f83a)

### Temporal Damage
This texture stores the damage in the same way as the previous damage map, but it fades over time. This is useful for animated effects applied immediately after damage is dealt that should disappear over time, such as the melted metal effect around a bullet hole. Again, the `BlastableComponent` will set the texture parameter `RT_FadingDamage` in the armor material to be this texture.

<p align="center">
   <img src="https://github.com/LDiazN/ArmorBlasting/assets/41093870/e3de042e-56d2-4d5f-8dc6-70d3005ae009" alt="Example of temporal damage"/>
</p>

# Implementation Details

The process of damaging the enemy armor looks like this:

1. We first trigger a damage event, this is implemented with the `BlastableComponent::Blast(FVector, float)` function we mentioned before. To damage the texture we need the damaged position and a radius.
2. Once the event is called, we want to check wich points in the surface of the armor are **inside a sphere** centered at the damage position and with the specified radius and mark them in the render targets. To do this we do a **texture unwrapping** for the armor mesh, the material used for unwrapping also marks which fragments lay inside the sphere.
3. Now that the surface is unwrapped, we take a **scene capture** twice, one for each render target.
4. The `TimeDamageRenderTarget`, the one that fades over time, also requires updating since the content changes every few milliseconds.
5. Now that the render targets are updated, these are used inside the armor material to display the damage sampling from the render targets.

In the following sections I explain this process more carefully.

## Mesh Unwrapping 

The general idea is that we want to create a damage map for our model surface that we can sample using texture coordinates. A more comprehensive guide on mesh unwrapping and painting can be found in this amazing tutorial from [kodeco](https://www.kodeco.com/6817-dynamic-mesh-painting-in-unreal-engine-4). However, the general outline of the process is as follows:

1. Create a material with two parameters: the position and radius of the damage sphere.
2. Mark a pixel as white if it is inside the damage sphere, and black otherwise.
3. Move the pixel to a horizontal plane in its corresponding position according to its texture coordinates using the world position offset output pin. This can be done by subtracting the pixel's position from its Absolute World Position and adding its texture coordinates.
4. Take a scene capture using a *Scene Capture Component*.

Note that one of the hardest parts of this process is getting the scene capture set up correctly. This requires careful configuration, as you may end up capturing the robot that should not be visible, or capturing the sky map. For this reason, I tried to set up all relevant configurations from the C++ class so that it's properly configured when used in a blueprint. The configuration for the scene capture component can be found in [this function](https://github.com/LDiazN/ArmorBlasting/blob/43e2e5c155df63f687b75b28f3e6f65034493a35/Source/ArmorBlasting/BlastableComponent.cpp#L217).

Another important factor to consider is that the mesh must meet certain requirements in order to be unwrapped using this approach. These requirements include:

* **Unique texture coordinates**: The mesh must have a different texture coordinate at every point. Otherwise, when the damage is being sampled, some points may show up damaged when nothing happened to them, which is confusing.
* **Precise physics**: The physics asset for the mesh should approximate the mesh as much as possible. This is because a ray can hit the physics asset further from the mesh, and trigger a hit but with no damage being recorded in the damage map since it was too far from the actual mesh surface. For this reason, I used a low-poly mesh for the armor, so that I can use complex collisions with reduced computational cost.
* **UV matching for each mesh**: Note that each armor part is its own independent mesh. However, in order to get a consistent unwrapping and damage mapping, each part must be aware of the other parts. This is so that they have a subset of UV coordinates that does not collide with other parts.
 
This is how the UVs look like in Blender for the robot armor. Every part shares the same UV map and texture so that there is no overlapping on unwrapping.

<p align="center">
   <img src="https://github.com/LDiazN/ArmorBlasting/assets/41093870/33aa7a2a-5e7d-4e4a-a296-599859144818" alt="UV Editing in Blender"/>
</p>

## Fading mesh update
The `TimeDamageRenderTarget` is a temporal damage map, so its content must be updated continuously over time. This update must be implemented as a function that is called repeatedly. For this reason, I start a timer on `BeginPlay` in the `BlastableComponent` that will update the render target every 10 milliseconds, at a rate of ~10 frames per second. It is important to not raise the update ratio too high, as this can cause the GPU to be overloaded with graphics calls.

In each draw call, I use the `UCanvas::K2_DrawMaterial` function to draw a simple material that just samples a texture. The output color of the material is the same texture color, but dimmed. The material output color is then drawn again in the same texture that was previously sampled. This way, the material will fade the texture color over time, which is what we want.

Note that for this to work properly, the texture render target that we are sampling and writing back must be configured with `RenderTarget->bNeedsTwoCopies = true`. Otherwise, the texture will be cleared before the material is computed, and then the texture sample will always return black.

## Armor Material

The armor material uses the information provided by the damage maps to display damage in the surface. In my case, I wanted to poke holes in the armor, so our first intuition is to bind 
the damage to the alpha transparency. So, we start with a standard texture sampling shader. However, in order to use transparency we have to set the **blend mode** for this material to **masked**, this will unlock the _opacity mask_ output pin in our material, and we can sample the damage maps with texture coordinates to get the transparency in this point. 

Then I encountered a problem: the inside of the mesh would be transparent due to backface culling. To solve this, I made the material two-sided. This means that the material will be rendered even if the backface of the mesh is facing the camera. You can use the _TwoSidedSign_ node to know in which side you are at any moment, and I use it to render the inside of the armor black. 

I also wanted to create a glowing ring around the hole, so I used three different channels in the damage map. The red channel stores the sphere with the default radius, the green channel stores a sphere that is 10% larger, and the blue channel stores a sphere that is 20% larger. I use the blue channel to implement the glow, since it will contain the hole entirely. This is also why you see a blue radius around damage zones in the damage maps.

To summarize, the most important configuration settings for this material are:

* **Blending mode**: The blending mode must be set to masked in order to implement holes.
* **Two-sided**: The material must be two-sided in order to render the inside of the mesh.
* **Different channels**: Different channels can be used to store different sizes of holes, which allows you to create a glowing ring around the hole.


# Known issues

The current implementation has some problems that can be improved in further iterations. These problems are as follows:

* **Visual aliasing in the armor body**: You might have already noticed that holes in the armor body have some pixelated borders, while holes in the helmet or arms are perfectly round. This is due to the way that Blender generated the automatic UV unwrapping. The armor texture is fairly low resolution (1024x1024) and the body part has a very small region of the texture compared to the size of its 3D object. For this reason, few pixels are assigned to the armor body, causing aliasing problems. This can be solved by manually setting the texture coordinates so that bigger parts of the object are assigned bigger regions of the texture.

<p align="center">
   <img src="https://github.com/LDiazN/ArmorBlasting/assets/41093870/02958438-e258-4c3e-9a3e-c5d359cd02fa" alt="Example of regions assigned to armor body in the entire texture" width="50%"/>
</p>

* **Temporal Damage Updating too much** : The temporal damage map requires multiple updates per second, which can be expensive if there are too many enemies in the scene. We can reduce this cost by changing our update function to only update the render target when there has been a new hit in the past few seconds. This would be helpful because the player will usually only be focusing on a few enemies at a time.

# Further improvements

There are a few improvements that we can make to the final result, which would make it even better. These improvements include:

* **Dynamic knockback**: This would give a lot of impact to our weapons by pushing back some parts of the robot depending on the bullets' impact position and direction. This would add more impact and destruction power to the player's weapon.
* **Better holes**: The current damage material implementation causes some regions of the armor to become floating islands of armor when there are too many holes. This is immersion breaking. We can improve this by changing the holes rendering to be more rough rather than a perfect hole. A more sophisticated solution would be to chop the disconnected parts altogether.
* **Armor blasting**: When an enough amount of damage has been made to a piece of armor, we could blast it entirely, completely exposing the internal robot structure. This could lead to cool gameplay mechanics.

