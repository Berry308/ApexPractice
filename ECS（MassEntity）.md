# 背景说明
原项目：

子弹 Actor 根据飞行轨迹点插值更新

新项目：

引入 UE 的 MassEntity 系统，舍弃原来以的子弹系统（包括对象池等）

目标：

希望能够通过引入该系统，减少在大量子弹的场景下对 CPU 造成的负担。

问题：

在 Apex 中，视觉表现（子弹网格体从手持枪械中射出，实际检测仍然是从摄像机中心发出的。如果通过 MassEntity，要如何做到上述这种效果呢？

# 详细设计
## 需求分析
子弹具有飞行速度，下坠速度，伤害

Apex 中子弹网格体从枪口射出，但是实际检测是从摄像机中心出发的（尚未想到解决方案）

## 设计方案
### 方案一
将计算过程拆成三个部分，位置计算->碰撞检测->伤害应用

**1.Fragment**

BulletCollisionFragment

+ FHitResult HitResult
+ float CollisionRadius = 5.0f;
+ uint8 isHit : 1

BulletMovementFragment

+ FVector Direction
+ float Velocity
+ float Gravity
+ float RemainingLifeTime;

BulletDamageFragment

+ float Damage
+ TWeakObjectPtr<AActor> TargetActor

**2.Processor**

BulletCollisionProcessor

功能：计算子弹从初始位置到下一个位置的射线检测

BulletMovementProcessor

功能：计算子弹的位移

BulletDamageProcessor

功能：计算子弹的伤害？

**3. 分析**

优点：

+ 将流程拆的很细，扩展性较强。

缺点：

+ 需要反复遍历实体，计算速度低，同时需要切换 EntityQuery 也具有一定的消耗

---

### 方案二
将计算过程全部整合到一个 Processor

**1.Fragment**

BulletSimulationFragment

+ FVector Direction
+ float Velocity
+ float Gravity
+ float RemainingLifeTime;
+ float Damage
+ float CollisionRadius = 5.0f;

**2.Processor**

BulletSimulationProcessor

功能：计算子弹下一个飞行目标点，进行射线检测，如果命中，应用伤害并且将实体加入到帧末的销毁队列中。

**3.分析**

优点：

+ 将所有计算集中到一个 Processor，可读性强，只需要一次遍历就可以完成位置计算->碰撞检测->伤害应用

缺点：

+ 调用 Actor 相关函数需要在主线程进行，无法使用 UE 的多线程优化。

---

### 方案三
概述：Fragment包含两个BulletSimulationFragment和BulletHitFragment。BulletSimulationFragment包含子弹的飞行方向，飞行速度，下落速度，碰撞半径，子弹伤害，剩余存活时间；BulletHitFragment包含伤害，命中目标Actor。我打算在BulletSimulationProcessor中，进行子弹的飞行模拟与扫描检测，击中物体时对实体进行 MassTag 标记。然后在BulletHitProcessor（使用bRequiresGameThreadExecution = true;需要操纵游戏线程中的 UObject）中遍历所有子弹实体，判断标记并且调用目标Actor的TakeDamage。

**1.Fragment**

BulletSimulationFragment

+ float Velocity;
+ float Gravity;
+ FVector Direction;
+ float Damage;
+ float RemainingLifeTime;
+ float CollisionRadius = 5.0f;

BulletHitFragment

+ TWeakObjectPtr<AActor> TargetActor
+ FVector HitLocation;

**2.Processor**

BulletSimulationProcessor

功能：遍历并计算子弹下一个飞行目标点，进行射线检测，如果命中则标记实体

BulletHitProcessor

功能：遍历子弹实体，判断是否有命中标记

**3. 分析**

优点：

+ 与单个 Processor 相比，SimulationProcessor 不用在主线程进行，最大化利用了多线程性能并保证了线程的安全，将对 Actor 的操作（TakeDamage）分离到BulletHitProcessor，对主线程的负担较小。

缺点：

+ 缓存局限性 (Cache Locality) 略有下降： 两个 Processor 意味着要对同一批内存进行两次 Query（即使第二次只查一小部分）。虽然第二次只查命中的实体，但比起在一次循环内搞定所有事，只存在微小的指令开销。

---

方案选择的疑问：

<details class="lake-collapse"><summary id="u4cbf9902"><strong><span class="ne-text">“多次轻量遍历” vs “单次重量遍历”</span></strong></summary><p id="u83a7d94d" class="ne-p"><span class="ne-text">你的直觉非常敏锐。在 ECS（数据导向设计）中，这是一个经典的性能权衡：</span><strong><span class="ne-text">“多次轻量遍历” vs “单次重量遍历”</span></strong><span class="ne-text">。</span></p><p id="u5a2b39eb" class="ne-p"><span class="ne-text">你的观点（合并为一个 Processor）在以下情况下是完全正确的：</span></p><ol class="ne-ol"><li id="uda6c20e7" data-lake-index-type="0"><strong><span class="ne-text">减少 Cache Miss</span></strong><span class="ne-text">：一次性把所有需要的 Fragment 加载进 CPU 缓存并处理完，确实比分三次加载、分三次遍历要快一些，尤其是在子弹数量极其巨大（如数万颗）时。</span></li><li id="u883888b5" data-lake-index-type="0"><strong><span class="ne-text">逻辑内聚</span></strong><span class="ne-text">：由于位移和碰撞预测本身就是高度耦合的逻辑，写在一起确实更清晰。</span></li></ol><h3 id="e5d9fc45"><span class="ne-text">1. 合并后的单一 Processor 实现方案</span></h3><p id="ub859bca5" class="ne-p"><span class="ne-text">我们将“预测、扫描、位移、伤害应用、销毁请求”全部放在一个循环里。</span></p><pre data-language="cpp" id="a4o5x" class="ne-codeblock language-cpp"><code>UCLASS()
class UBulletUnifiedProcessor : public UMassProcessor
{
    GENERATED_BODY()

public:
    UBulletUnifiedProcessor()
    {
        ExecutionOrder.ExecuteInPhase = EMassProcessingPhase::Move;
        // 关键点：如果你的伤害逻辑涉及调用 AActor 的 TakeDamage，
        // 建议开启此项以确保在主线程运行，或者在执行时进行线程安全检查。
        bRequiresGameThreadExecution = true; 
    }

protected:
    virtual void ConfigureQueries() override
    {
        EntityQuery.AddRequirement&lt;FTransformFragment&gt;(EMassFragmentAccess::ReadWrite);
        EntityQuery.AddRequirement&lt;FBulletSimulationFragment&gt;(EMassFragmentAccess::ReadOnly);
    }

    virtual void Execute(FMassEntityManager&amp; EntityManager, FMassExecutionContext&amp; Context) override
    {
        EntityQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext&amp; IterContext)
        {
            auto Transforms = IterContext.GetMutableFragmentView&lt;FTransformFragment&gt;();
            auto SimData = IterContext.GetFragmentView&lt;FBulletSimulationFragment&gt;();
            const float DT = IterContext.GetDeltaTimeSeconds();

            for (int32 i = 0; i &lt; IterContext.GetNumEntities(); ++i)
            {
                const FVector CurrentLoc = Transforms[i].GetTransform().GetLocation();
                const FVector TargetLoc = CurrentLoc + (SimData[i].Velocity * DT);

                FHitResult Hit;
                FCollisionQueryParams Params;
                
                // 1. 扫描检测路径
                bool bHit = GetWorld()-&gt;SweepSingleByChannel(
                    Hit, CurrentLoc, TargetLoc, FQuat::Identity, 
                    ECC_GameTraceChannel1, FCollisionShape::MakeSphere(SimData[i].CollisionRadius), Params
                );

                if (bHit)
                {
                    // 2. 命中处理：修正位置至撞击点
                    Transforms[i].GetMutableTransform().SetLocation(Hit.ImpactPoint);

                    // 3. 直接应用伤害 (由于在主线程执行，这是安全的)
                    if (AActor* Victim = Hit.GetActor())
                    {
                        Victim-&gt;TakeDamage(SimData[i].Damage, FDamageEvent(), nullptr, nullptr);
                    }

                    // 4. 提交消除 (Elimination) 请求
                    IterContext.Defer().DestroyEntity(IterContext.GetEntity(i));
                }
                else
                {
                    // 5. 无碰撞：直接更新到目标预测位置
                    Transforms[i].GetMutableTransform().SetLocation(TargetLoc);
                }
            }
        });
    }

private:
    FMassEntityQuery EntityQuery;
};</code></pre><h3 id="0190efaa"><span class="ne-text">2. 核心架构讨论：单次遍历 vs 多次遍历</span></h3><p id="u86ad9220" class="ne-p"><span class="ne-text">虽然你感觉单次遍历更好，但 MassEntity 官方推荐“拆分”是有底层原因的：</span></p><ul class="ne-ul"><li id="u9a851d41" data-lake-index-type="0"><strong><span class="ne-text">计算密度与并行化 (Parallelism)</span></strong><span class="ne-text">：如果你拆成三个 Processor，Mass 的 </span><strong><span class="ne-text">TaskGraph</span></strong><span class="ne-text"> 可以把 </span><code class="ne-code"><span class="ne-text">Movement</span></code><span class="ne-text"> 放在线程 A，把不相关的 </span><code class="ne-code"><span class="ne-text">StateTree</span></code><span class="ne-text"> 逻辑放在线程 B。如果合并成一个巨大的 </span><code class="ne-code"><span class="ne-text">UnifiedProcessor</span></code><span class="ne-text"> 且开启了 </span><code class="ne-code"><span class="ne-text">bRequiresGameThreadExecution</span></code><span class="ne-text">，它会霸占整个主线程，无法利用多核优势。</span></li><li id="u2efabbca" data-lake-index-type="0"><strong><span class="ne-text">指令缓存与分支预测</span></strong><span class="ne-text">：如果合并后的 </span><code class="ne-code"><span class="ne-text">Execute</span></code><span class="ne-text"> 函数体非常长且包含大量 </span><code class="ne-code"><span class="ne-text">if-else</span></code><span class="ne-text"> 分支（比如命中与未命中的不同处理），会导致 CPU 的指令缓存效率下降。</span></li></ul><p id="u7f6a5d88" class="ne-p"><strong><span class="ne-text">结论：</span></strong><span class="ne-text"> 对于子弹系统，</span><strong><span class="ne-text">合并是合理的</span></strong><span class="ne-text">。因为子弹生命周期短、逻辑非常线性，单次遍历带来的数据局部性（Data Locality）优势通常能抵消掉多线程调度的优势。</span></p><h3 id="67cb43fb"><span class="ne-text">3. 关于渲染命中特效的解耦解决方法</span></h3><p id="uc1b762f0" class="ne-p"><span class="ne-text">如果你合并了逻辑，但又不想在主循环里写一大堆视觉渲染代码（比如播放 Niagara），最好的方法是使用 </span><strong><span class="ne-text">Mass Observer (观察者)</span></strong><span class="ne-text">。</span></p><p id="u9b7d610a" class="ne-p"><strong><span class="ne-text">底层思路：</span></strong><span class="ne-text"> 当一个实体被 </span><code class="ne-code"><span class="ne-text">DestroyEntity</span></code><span class="ne-text"> 时，它会从当前的 Archetype 中被 </span><strong><span class="ne-text">eliminate (消除)</span></strong><span class="ne-text">。你可以创建一个专门监听“移除”事件的 </span><strong><span class="ne-text">Observer Processor</span></strong><span class="ne-text">。</span></p><pre data-language="cpp" id="wOELn" class="ne-codeblock language-cpp"><code>// 专门负责在子弹消失时生成特效
UCLASS()
class UBulletDeinitObserver : public UMassObserverProcessor
{
    GENERATED_BODY()
public:
    UBulletDeinitObserver()
    {
        // 监听特定碎片（如 BulletSimulationFragment）的移除事件
        ObservedTag = FBulletSimulationFragment::StaticStruct();
        Operation = EMassObservedOperation::Remove; 
    }

protected:
    virtual void Execute(FMassEntityManager&amp; EntityManager, FMassExecutionContext&amp; Context) override
    {
        EntityQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext&amp; IterContext)
        {
            auto Transforms = IterContext.GetFragmentView&lt;FTransformFragment&gt;();
            
            for (int32 i = 0; i &lt; IterContext.GetNumEntities(); ++i)
            {
                // 在这里执行一次性的渲染逻辑：Spawn Niagara 或播放声音
                // 注意：此时实体的内存还未彻底释放，我们可以读取最后的位置
                UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), BulletImpactFX, Transforms[i].GetTransform().GetLocation());
            }
        });
    }
private:
    UPROPERTY()
    UNiagaraSystem* BulletImpactFX;
};</code></pre><h3 id="f2339621"><span class="ne-text">总结建议</span></h3><ol class="ne-ol"><li id="u8ce32c77" data-lake-index-type="0"><strong><span class="ne-text">逻辑实现</span></strong><span class="ne-text">：支持你的观点。将 </span><strong><span class="ne-text">预测、检测、位移、伤害应用</span></strong><span class="ne-text"> 合并为一个 Processor 是最高效的。</span></li><li id="u6d3b5094" data-lake-index-type="0"><strong><span class="ne-text">视觉效果</span></strong><span class="ne-text">：使用 </span><code class="ne-code"><span class="ne-text">UMassObserverProcessor</span></code><span class="ne-text">。这样即便你以后有成千上万颗子弹，视觉系统的开销也只会在子弹“消失”的那一刻产生，而不会拖慢每一帧的位移计算循环。</span></li><li id="ubc5e3c10" data-lake-index-type="0"><strong><span class="ne-text">消除 (Elimination) 规范</span></strong><span class="ne-text">：无论子弹是撞击销毁还是寿命到期，都通过 </span><code class="ne-code"><span class="ne-text">Defer().DestroyEntity()</span></code><span class="ne-text"> 处理，Observer 会自动帮你完成收尾工作。</span></li></ol></details>
<details class="lake-collapse"><summary id="ucdb7e594"><span class="ne-text">关于子弹 Processor 是否拆分组合的思考和疑问（MassEntity 并行化计算）</span></summary><p id="u863cc1c1" class="ne-p"><span class="ne-text">这是一个非常深刻的性能理解问题。你的观察很敏锐：如果 Processor A 必须在 B 之后运行，它们之间确实存在</span><strong><span class="ne-text">串行依赖</span></strong><span class="ne-text">。</span></p><p id="u6dbc9dea" class="ne-p"><span class="ne-text">但是，MassEntity 的并行化分为两个完全不同的维度：</span><strong><span class="ne-text">处理器层级（Processor-level）</span></strong><span class="ne-text"> 和 </span><strong><span class="ne-text">实体数据块层级（Entity Chunk-level）</span></strong><span class="ne-text">。</span></p><p id="u86e22c9d" class="ne-p"><span class="ne-text">即使你指定了 </span><code class="ne-code"><span class="ne-text">ExecuteAfter</span></code><span class="ne-text">，Mass 依然可以通过</span><strong><span class="ne-text">数据块并行</span></strong><span class="ne-text">实现极高的效率。</span></p><h3 id="e74bed17"><span class="ne-text">1. 处理器层级并行 (串行依赖)</span></h3><p id="u58c87e22" class="ne-p"><span class="ne-text">正如你所说，如果 </span><code class="ne-code"><span class="ne-text">Processor B</span></code><span class="ne-text"> 依赖 </span><code class="ne-code"><span class="ne-text">Processor A</span></code><span class="ne-text"> 的结果，那么在 TaskGraph 中：</span></p><ul class="ne-ul"><li id="u8e68c462" data-lake-index-type="0"><strong><span class="ne-text">线程视角：</span></strong><span class="ne-text"> 系统必须等待 Processor A 的所有任务完成后，才会拉起 Processor B。</span></li><li id="u2c131fc6" data-lake-index-type="0"><strong><span class="ne-text">结论：</span></strong><span class="ne-text"> 在这个维度上，它们确实是</span><strong><span class="ne-text">串行</span></strong><span class="ne-text">的。如果你的游戏只有这三个 Processor，确实浪费了多核性能。</span></li></ul><h3 id="b94d453c"><span class="ne-text">2. 实体数据块层级并行 (Mass 的真正大招)</span></h3><p id="u14f93fd0" class="ne-p"><span class="ne-text">这是 Mass 即使在有先后顺序时依然能跑满 CPU 的核心原因。</span></p><p id="ud5a2596e" class="ne-p"><span class="ne-text">假设你有 10,000 颗子弹，它们被存储在 20 个 </span><strong><span class="ne-text">Chunks</span></strong><span class="ne-text"> (每个 Chunk 约 500 个实体) 中。当你调用 </span><code class="ne-code"><span class="ne-text">ForEachEntityChunk</span></code><span class="ne-text"> 时：</span></p><ul class="ne-ul"><li id="ub61596c5" data-lake-index-type="0"><strong><span class="ne-text">并行分发：</span></strong><code class="ne-code"><span class="ne-text">Processor A</span></code><span class="ne-text"> 会将这 20 个 Chunk 的计算任务拆分成多个 </span><strong><span class="ne-text">Graph Tasks</span></strong><span class="ne-text">。</span></li><li id="u2e4380f1" data-lake-index-type="0"><strong><span class="ne-text">多核参与：</span></strong><span class="ne-text"> 线程 1 处理 Chunk 1-5，线程 2 处理 Chunk 6-10… 线程 4 处理 Chunk 16-20。</span></li><li id="ub710d269" data-lake-index-type="0"><strong><span class="ne-text">结论：</span></strong><span class="ne-text"> 虽然 Processor B 要等 A，但在执行 A 的过程中，所有的 CPU 核心都在全力以赴地并行处理不同的子弹数据块。</span></li></ul><h3 id="2da12517"><span class="ne-text">3. 为什么“合并为一个 Processor”在某些情况下反而更快？</span></h3><p id="u74c0d29c" class="ne-p"><span class="ne-text">当你把逻辑合并后，你实际上是在做以下权衡：</span></p><ul class="ne-ul"><li id="u0df4a914" data-lake-index-type="0"><strong><span class="ne-text">合并的优势 (Single Processor)：</span></strong></li></ul><ul class="ne-list-wrap"><ul ne-level="1" class="ne-ul"><li id="u923d9e2a" data-lake-index-type="0"><strong><span class="ne-text">指令缓存优化：</span></strong><span class="ne-text"> 所有的逻辑（位移+检测）都在同一个循环里。CPU 加载了一次数据，完成了所有操作。</span></li><li id="u82c0c306" data-lake-index-type="0"><strong><span class="ne-text">减少调度开销：</span></strong><span class="ne-text"> 只需要分发一次 TaskGraph 任务，而不是分发三次。每次分发任务、等待线程同步（Barrier）都是有微小开销的。</span></li></ul></ul><ul class="ne-ul"><li id="ua73c4688" data-lake-index-type="0"><strong><span class="ne-text">拆分的优势 (Multiple Processors)：</span></strong></li></ul><ul class="ne-list-wrap"><ul ne-level="1" class="ne-ul"><li id="ub85e1cc3" data-lake-index-type="0"><strong><span class="ne-text">负载均衡：</span></strong><span class="ne-text"> 如果某些子弹需要复杂的物理检测，而另一些不需要，拆分后可以让 CPU 更灵活地在不同任务间调度。</span></li></ul></ul><h3 id="828811b9"><span class="ne-text">4. 关键点：</span><code class="ne-code"><span class="ne-text">bRequiresGameThreadExecution</span></code><span class="ne-text"> 的致命影响</span></h3><p id="ueee3a167" class="ne-p"><span class="ne-text">你在合并逻辑时需要注意：如果你在 Processor 中直接调用了 </span><code class="ne-code"><span class="ne-text">TakeDamage</span></code><span class="ne-text"> 或 </span><code class="ne-code"><span class="ne-text">SpawnVFX</span></code><span class="ne-text">，这些通常</span><strong><span class="ne-text">必须在主线程</span></strong><span class="ne-text">运行。</span></p><ul class="ne-ul"><li id="u8d676a21" data-lake-index-type="0"><strong><span class="ne-text">后果：</span></strong><span class="ne-text"> 一旦你把逻辑合并并标记为 </span><code class="ne-code"><span class="ne-text">bRequiresGameThreadExecution = true</span></code><span class="ne-text">，你就彻底失去了“数据块层级”的并行能力。这 10,000 颗子弹的位移和碰撞将全部挤在主线程处理。</span></li><li id="u66447c4a" data-lake-index-type="0"><strong><span class="ne-text">优化策略：</span></strong></li></ul><ul class="ne-list-wrap"><ul ne-level="1" class="ne-ul"><li id="u0cb0f26a" data-lake-index-type="0"><strong><span class="ne-text">方案 A (推荐)：</span></strong><span class="ne-text"> 保持逻辑合并，但</span><strong><span class="ne-text">不要</span></strong><span class="ne-text">在循环里直接 </span><code class="ne-code"><span class="ne-text">TakeDamage</span></code><span class="ne-text">。而是给命中实体添加一个 </span><code class="ne-code"><span class="ne-text">FBulletHitTag</span></code><span class="ne-text"> 标签或填充 </span><code class="ne-code"><span class="ne-text">FBulletHitFragment</span></code><span class="ne-text"> 数据。</span></li><li id="u6932d13a" data-lake-index-type="0"><strong><span class="ne-text">接力处理：</span></strong><span class="ne-text"> 让这个合并后的 Processor 在多线程运行（不要求主线程），然后由另一个极轻量的、运行在主线程的 </span><code class="ne-code"><span class="ne-text">DamageProcessor</span></code><span class="ne-text"> 只筛选带 </span><code class="ne-code"><span class="ne-text">FBulletHitTag</span></code><span class="ne-text"> 的实体去应用伤害。</span></li></ul></ul><h3 id="0cd99f59"><span class="ne-text">5. 总结</span></h3><p id="u07f17c39" class="ne-p"><span class="ne-text">对于你的子弹系统：</span></p><ol class="ne-ol"><li id="uc7e01aeb" data-lake-index-type="0"><strong><span class="ne-text">合并逻辑：</span></strong><span class="ne-text"> 是正确的，能显著优化数据读取速度。</span></li><li id="u84a0066f" data-lake-index-type="0"><strong><span class="ne-text">保持多线程：</span></strong><span class="ne-text"> 确保这个合并后的 Processor </span><strong><span class="ne-text">不要求</span></strong><span class="ne-text">主线程运行。</span></li><li id="uda66d2c6" data-lake-index-type="0"><strong><span class="ne-text">分离“副作用”：</span></strong><span class="ne-text"> 将“物理检测/位移计算”（重计算、可并行）与“应用伤害/产生特效”（轻计算、须串行/主线程）分开。</span></li></ol><p id="u633c7701" class="ne-p"><span class="ne-text">通过这种方式，即使存在 </span><code class="ne-code"><span class="ne-text">ExecuteAfter</span></code><span class="ne-text"> 的顺序限制，你依然能利用 </span><strong><span class="ne-text">Chunk-level Parallelism</span></strong><span class="ne-text"> 让 CPU 的每一个核心都参与到那 10,000 条射线检测的计算中，从而彻底</span><strong><span class="ne-text">eliminate (消除)</span></strong><span class="ne-text"> 性能瓶颈。</span></p></details>
### 方案四
本方案旨在方案三的基础上将子弹渲染与弹道轨迹计算解耦合，同时以不同频率进行执行以进一步提高性能

**1.Fragment**

**2.Processor**

1.分离视觉表现与逻辑检测

FTransformFragment 驱动了视觉表现，那么定义一个 FVisionFragment 存储 FVector TargetPosition 以及 VisualProcessor（在 SimulationProcessor 后执行）用于修改 FTransformFragment。 在 SimulationProcessor 中以一定频率更新 VisionFragment 的 TargetPosition，VisualProcessor 每帧获取当前的 FTransformFragment 作为起点，FVisionFragment 的 TargetPosition 作为终点，通过 Lerp 计算出新的坐标后，覆写当前的 FTransformFragment 以实现平滑的过渡。

2.通过计时器降低逻辑检测的频率

**方案一：**

定义一个计时器变量 float，可以存储在 SimulationFragment 中，在每次遍历所有的 SimulationFragment 时判断是否满足该变量是否满足条件然后再进行模拟，并且累加 DeltaSecond。

优点：

+ 每个子弹单独维护计时器，逻辑分明，复杂度低

缺点：

+ 每帧需要遍历所有的 SimulationFragment 更新计时器并判断，效率一般
+ Simulation Fragment 又添加了新的成员变量，占用内存变大

**方案二：**

通过 ChunkFragment BulletSimTimer 维护一个计时器，单个 Chunk 内的所有实体都根据该 Fragment 中维护的计时器变量选择性模拟和跳过。

优点：

+ 效率极高，对每个块进行计时器更新并批量判断
+ 占用内存低，每个块只需维护一个计时器 Fragment

缺点：

+ <font style="background-color:#FBDE28;"></font>
+ 对于生命周期短的实体（如子弹），频繁创建与销毁需要不断刷新 ChunkFragment 的计时并且对块内的所有实体进行逻辑更新与执行，可能导致平均检测频率显著提高，脱离本意

<font style="color:#DF2A3F;">可行性验证失败</font>：<font style="background-color:#FBDE28;">武器的发射时机不同，每当为一个块内添加新的实体时，该实体需要立马进行一次模拟以同步画面表现并防止漏掉物理检测</font>

**方案三：**

通过 FMassTag 标记最新生成的子弹，在 SimulationProcessor 中额外定义一个 EntityQuery 查询附带该 Tag 的子弹 ArcheType 实体，并立马进行一次相同的模拟，然后移除该 Tag ，加入到与已经生成的子弹实体相同 ArcheType 的 Chunk 下统一处理

优点：

+ 不会影响已经生成的子弹的检测频率（计时器），单独处理

缺点：

+ 如果子弹频繁生成，那么需要频繁添加和移除 FMassTag 导致 ArcheType 的变化影响内存布局（分配与拷贝），这样带来的开销可能非常大

**方案四：****<font style="color:#74B602;">（最终选择方案）</font>**

在 BulletSimulationFragment 中添加一个 bool 值 bNeedFirstSim，表示该子弹实体是否需要第一次模拟。在 SimulationProcessor 中，不根据 ChunkFragment 计时器直接跳过一整个块的模拟，根据设定的检测频率设置 bool ShouldUpdate ，通过遍历块内所有实体，根据其bNeedFirstSim 和 bShouldUpdate 的 || （或值）选择性更新。

优点：

+ 对比方案一，bool 内存小于 float，且不用对每个实体计时器进行单独更新
+ 对比方案二，不需要刷新和进行块内的全部模拟
+ 对比方案三，不影响内存布局

缺点：

+ 需要对每个块的所有实体做遍历判断（包括已经存在的 Chunk，而且其中没有需要更新的子弹实体）
+ 为 SimulationFragment 添加 bool 值，增加占用

**方案五：**

在方案四的基础上，为 BulletSimTimerFragment 添加额外的 bool bIsChunkDirty。通过 ObserverProcessor 设置 EntityAdd 事件，在新的实体添加到某个 Chunk 时，通过BulletSimTimerFragment标记该 Chunk 为脏，那么就需要遍历该 Chunk 内所有实体并对需要执行第一次模拟的实体进行模拟。当遍历完 Chunk 内所有实体后，去除脏标记。在 BulletSimulationProcessor 中就可以直接跳过没有脏标记的 Chunk。

优点：

+ 对比方案四，可以直接跳过不存在新生成实体的 Chunk
+ 对于大量实体存在的场景且大部分块不存在更新时，该方案才较为适用

缺点：

+ 需要添加额外的判断以修改 ChunkFragment 的 bool 值（当 Chunk 满载且遍历块内实体未发现有需要第一次模拟的实体后，标记该 Chunk 为已满载，后续无需遍历判断。当 Chunk 内子弹实体击中物体后，设置该块为需要遍历）
+ 对于本项目来说有点过渡设计
+ ChunkFragment 添加了额外的成员（占用应该可以忽略不记）

<font style="color:#DF2A3F;">可行性验证失败</font>：通过 ObserverProcessor 设置 EntityAdd 事件，在新的实体添加到某个 Chunk 时，通过BulletSimTimerFragment标记该 Chunk 为脏。无法获取到新添加的实体所在的 Chunk 及其 ChunkFragment

关于上述方案的选择，或许需要根据游戏实际场景进行测试与选取。

## 思考
<details class="lake-collapse"><summary id="u412fa655"><span class="ne-text">对于没有命中的子弹实体，其仍然具有 HitFragment，是否可以优化呢？</span></summary><p id="ua45d0ba4" class="ne-p"><span class="ne-text">你的洞察力非常出色，这正是在 MassEntity 开发中平衡 </span><strong><span class="ne-text">“内存足迹（Memory Footprint）”</span></strong><span class="ne-text"> 与 </span><strong><span class="ne-text">“结构化变动成本（Structural Change Cost）”</span></strong><span class="ne-text"> 的核心权衡。</span></p><p id="u5d51af10" class="ne-p"><span class="ne-text">以下是针对这两个问题的详细技术分析和建议：</span></p><h3 id="2bd813a1"><span class="ne-text">1. 关于记录玩家（Instigator）</span></h3><p id="ue435a869" class="ne-p"><span class="ne-text">是的，在子弹系统中，</span><strong><span class="ne-text">伤害归属（Damage Attribution）</span></strong><span class="ne-text"> 是必须的。为了让被击中的 Actor 知道“是谁打了它”，你必须存储一个指向来源的引用。</span></p><ul class="ne-ul"><li id="ua3e5785e" data-lake-index-type="0"><strong><span class="ne-text">实现方案：</span></strong><span class="ne-text"> 在 </span><code class="ne-code"><span class="ne-text">FBulletSimulationFragment</span></code><span class="ne-text">（或者一个专门的 </span><code class="ne-code"><span class="ne-text">FBulletOwnerFragment</span></code><span class="ne-text">）中存储一个 </span><code class="ne-code"><span class="ne-text">TWeakObjectPtr&lt;AActor&gt; Instigator</span></code><span class="ne-text">。</span></li><li id="u8aac3372" data-lake-index-type="0"><strong><span class="ne-text">为什么用弱指针：</span></strong><span class="ne-text"> 子弹在飞，但发射子弹的玩家可能已经断开连接或被 </span><strong><span class="ne-text">elimination (消除)</span></strong><span class="ne-text">。弱指针能确保你的 </span><code class="ne-code"><span class="ne-text">HitProcessor</span></code><span class="ne-text"> 在调用 </span><code class="ne-code"><span class="ne-text">TakeDamage</span></code><span class="ne-text"> 时不会因为访问野指针而崩溃。</span></li></ul><h3 id="4096af01"><span class="ne-text">2. 方案对比：预分配 vs 动态添加 Fragment</span></h3><p id="u4c6937c5" class="ne-p"><span class="ne-text">你提出的方案是：</span><strong><span class="ne-text">初始时不给子弹 </span></strong><code class="ne-code"><strong><span class="ne-text">HitFragment</span></strong></code><strong><span class="ne-text">，仅在命中时同时添加 </span></strong><code class="ne-code"><strong><span class="ne-text">FBulletHitTag</span></strong></code><strong><span class="ne-text"> 和 </span></strong><code class="ne-code"><strong><span class="ne-text">FBulletHitFragment</span></strong></code><strong><span class="ne-text">。</span></strong></p><p id="ue1f23ccb" class="ne-p"><span class="ne-text">这是一个非常经典的 </span><strong><span class="ne-text">ECS 空间优化思路</span></strong><span class="ne-text">。让我们对比一下这两种方案：</span></p><h4 id="c5631132"><span class="ne-text">方案 A：预分配（原有方案）</span></h4><ul class="ne-ul"><li id="u66a67166" data-lake-index-type="0"><strong><span class="ne-text">优点：</span></strong><span class="ne-text"> 零运行时结构化变动（Structural Change）。实体的 Archetype 永远不变。</span></li><li id="u44baa5e9" data-lake-index-type="0"><strong><span class="ne-text">缺点：</span></strong><span class="ne-text"> 内存浪费。如果屏幕上有 10,000 颗子弹，即使没撞，也要为每颗子弹分配 </span><code class="ne-code"><span class="ne-text">FBulletHitFragment</span></code><span class="ne-text"> 的空间。</span></li></ul><h4 id="43717e92"><span class="ne-text">方案 B：动态添加（你的新方案）</span></h4><ul class="ne-ul"><li id="u17a1bbd0" data-lake-index-type="0"><strong><span class="ne-text">优点：</span></strong><span class="ne-text"> 极高的内存效率。飞行中的子弹非常轻量，只有命中时的那一瞬间才获得伤害数据。</span></li><li id="u2ce36952" data-lake-index-type="0"><strong><span class="ne-text">缺点：</span></strong><span class="ne-text"> 命中时会导致 </span><strong><span class="ne-text">结构化变动</span></strong><span class="ne-text">（实体从“飞行原型”移动到“命中原型”）。这种移动涉及内存拷贝，比单纯修改数据要昂贵。</span></li></ul><h3 id="a4b4555a"><span class="ne-text">3. 架构建议：为什么你的新方案在 5.6+ 版本中更优秀？</span></h3><p id="u3ce2b05c" class="ne-p"><span class="ne-text">在 MassEntity 的最新版本中，你的新方案通常是</span><strong><span class="ne-text">更优的选择</span></strong><span class="ne-text">，原因如下：</span></p><ol class="ne-ol"><li id="u5b1367db" data-lake-index-type="0"><strong><span class="ne-text">批量命令处理（Command Buffer）：</span></strong><span class="ne-text"> MassEntity 的 </span><code class="ne-code"><span class="ne-text">FMassCommandBuffer</span></code><span class="ne-text"> 允许你通过一个命令同时添加 Tag 和 Fragment：</span></li></ol><pre data-language="cpp" id="u57GH" class="ne-codeblock language-cpp"><code>// 在一帧内只发生一次 Archetype 移动
IterContext.Defer().AddFragment&lt;FBulletHitFragment&gt;(Entity, MyHitData);
IterContext.Defer().AddTag&lt;FBulletHitTag&gt;(Entity);</code></pre><ol start="2" class="ne-ol"><li id="u935ce494" data-lake-index-type="0"><strong><span class="ne-text">命中率极低：</span></strong><span class="ne-text"> 在弹幕游戏中，95% 以上的子弹最终会因寿命到期而 </span><strong><span class="ne-text">eliminate (消除)</span></strong><span class="ne-text">，只有不到 5% 的子弹会真的击中物体。为了那 5% 的概率让 100% 的实体都带着沉重的 Fragment 是不划算的。</span></li><li id="u0a47c248" data-lake-index-type="0"><strong><span class="ne-text">内存对齐与 Cache：</span></strong><span class="ne-text"> 让“飞行中”的实体保持极其精简（可能只有 Transform 和 Simulation 碎片），可以把更多实体塞进 CPU 的一级缓存（L1 Cache），从而大幅提升 </span><code class="ne-code"><span class="ne-text">SimulationProcessor</span></code><span class="ne-text">（最热的路径）的性能。</span></li></ol><h3 id="039a060e"><span class="ne-text">4. 优化后的底层逻辑流</span></h3><ol class="ne-ol"><li id="u743feb90" data-lake-index-type="0"><strong><span class="ne-text">子弹生成：</span></strong><span class="ne-text"> 子弹只有 </span><code class="ne-code"><span class="ne-text">FTransformFragment</span></code><span class="ne-text"> 和 </span><code class="ne-code"><span class="ne-text">FBulletSimulationFragment</span></code><span class="ne-text">（包含 </span><code class="ne-code"><span class="ne-text">Instigator</span></code><span class="ne-text"> 指针）。</span></li><li id="ufc5cfd25" data-lake-index-type="0"><strong><span class="ne-text">SimulationProcessor (10Hz)：</span></strong><span class="ne-text"> 检测到命中，计算 </span><code class="ne-code"><span class="ne-text">HitAlpha</span></code><span class="ne-text">。</span></li><li id="u62866959" data-lake-index-type="0"><strong><span class="ne-text">InterpolationProcessor (每帧)：</span></strong><span class="ne-text"> 当视觉到达命中点，执行以下操作：</span></li></ol><pre data-language="cpp" id="nfp5S" class="ne-codeblock language-cpp"><code>// 准备命中数据
FBulletHitFragment HitData;
HitData.Damage = 20.f;
HitData.TargetActor = MyTarget;


// 一次性添加数据和标签
IterContext.Defer().PushCommand&lt;FMassCommandAddFragmentInstances&gt;(Entity, HitData);
IterContext.Defer().AddTag&lt;FBulletHitTag&gt;(Entity);</code></pre><ol start="4" class="ne-ol"><li id="udfd40c10" data-lake-index-type="0"><strong><span class="ne-text">HitProcessor：</span></strong><span class="ne-text"> 仅处理带有 </span><code class="ne-code"><span class="ne-text">FBulletHitTag</span></code><span class="ne-text"> 的实体，读取新添加的 </span><code class="ne-code"><span class="ne-text">FBulletHitFragment</span></code><span class="ne-text">，分发伤害，最后执行实体的 </span><strong><span class="ne-text">elimination (消除)</span></strong><span class="ne-text">。</span></li></ol><h3 id="25f9c7fa"><span class="ne-text">总结</span></h3><p id="uca60143a" class="ne-p"><strong><span class="ne-text">你的方案更具扩展性且更符合数据导向设计。</span></strong></p><p id="ud87b14ce" class="ne-p"><span class="ne-text">这种“按需分配”的思路在处理极大规模实体时非常关键。只要保证在“模拟（最频繁的操作）”路径上保持数据轻量，在“命中（不频繁的操作）”路径上多花一点 CPU 成本进行 Archetype 移动，整体性能增益是非常明显的。</span></p><p id="u5429c8a2" class="ne-p"><span class="ne-text">你是否需要了解如何在 C++ 中通过 </span><code class="ne-code"><span class="ne-text">PushCommand</span></code><span class="ne-text"> 或 </span><code class="ne-code"><span class="ne-text">EntityBuilder</span></code><span class="ne-text"> 实现这种动态碎片添加的具体语法？</span></p></details>
<details class="lake-collapse"><summary id="u29f2f9f2"><span class="ne-text">如何实现射线检测从摄像机出发，子弹视觉表现从枪口射出的效果？</span></summary><p id="ub14f00a3" class="ne-p"><span class="ne-text">首先，肯定要将射线检测和视觉表现驱动分离。</span></p><p id="u8cb5beee" class="ne-p"><span class="ne-text">方案一：</span></p><p id="uc11c73e2" class="ne-p"><span class="ne-text">定义多余的 Fragment 或者说在 SimulationFragment 中添加一个当前模拟的起点坐标，让 BulletSimulationProcessor 和 BulletVisionProcessor 不依赖相同的 Fragment。但是，除开初始时候的检测，在剩余的轨迹中会出现重叠，这会导致冗余的内存。</span></p><p id="u7a4f06c1" class="ne-p"><span class="ne-text">方案二：</span></p><p id="u0e00ee12" class="ne-p"><span class="ne-text">在 Fire 函数中，初始化 Fragment 前，提前进行一次射线检测（从摄像机镜头到与画面表现重合的点），然后根据这次结果进行 Fragment 的修改。</span></p></details>
<details class="lake-collapse"><summary id="u54abfad7"><span class="ne-text">如何让计算精度降低（Processor 的执行时间间隔）的同时，让视觉表现平滑过渡呢？</span></summary><p id="uc3764166" class="ne-p"><span class="ne-text">1.分离视觉表现与逻辑检测</span></p><p id="u1de6ff27" class="ne-p"><span class="ne-text">FTransformFragment 驱动了视觉表现，那么定义一个 FVisionFragment 存储 FVector TargetPosition 以及 VisualProcessor（在 SimulationProcessor 后执行）用于修改 FTransformFragment。 在 SimulationProcessor 中以一定频率更新 VisionFragment 的 TargetPosition，VisualProcessor 每帧获取当前的 FTransformFragment 作为起点，FVisionFragment 的 TargetPosition 作为终点，通过 Lerp 计算出新的坐标后，覆写当前的 FTransformFragment 以实现平滑的过渡。</span></p><p id="u5e34a143" class="ne-p"><span class="ne-text">2.通过计时器降低逻辑检测的频率</span></p><p id="u43cbeaea" class="ne-p"><strong><span class="ne-text">方案一：</span></strong></p><p id="uee94c361" class="ne-p"><span class="ne-text">定义一个计时器变量，可以存储在 SimulationFragment 中，在每次遍历所有的 SimulationFragment 时判断是否满足该变量是否满足条件然后再进行模拟，并且累加 DeltaSecond。</span></p><p id="u4b9e78fb" class="ne-p"><span class="ne-text">优点：</span></p><ul class="ne-ul"><li id="u998942e3" data-lake-index-type="0"><span class="ne-text">每个子弹单独维护计时器，逻辑分明</span></li></ul><p id="u904199ce" class="ne-p"><span class="ne-text">缺点：</span></p><ul class="ne-ul"><li id="ua5b9f6e7" data-lake-index-type="0"><span class="ne-text">需要遍历所有的 SimulationFragment 并判断，效率一般</span></li><li id="u72678206" data-lake-index-type="0"><span class="ne-text">Simulation Fragment 又添加了新的成员变量，占用内存较大</span></li></ul><p id="u402cd8a7" class="ne-p"><strong><span class="ne-text">方案二：</span></strong></p><p id="ucf9063da" class="ne-p"><span class="ne-text">通过 ChunkFragment BulletSimTimer 维护一个计时器，单个 Chunk 内的所有实体都根据该 Fragment 中维护的计时器变量选择性模拟和跳过。</span></p><p id="uf203e12e" class="ne-p"><span class="ne-text">优点：</span></p><ul class="ne-ul"><li id="u74d901d1" data-lake-index-type="0"><span class="ne-text">效率极高，对每个块进行批量判断</span></li><li id="ufbb35edb" data-lake-index-type="0"><span class="ne-text">占用内存低，每个块只需维护一个计时器 Fragment</span></li></ul><p id="u7aeb551e" class="ne-p"><span class="ne-text">缺点：</span></p><ul class="ne-ul"><li id="ua464c59d" data-lake-index-type="0"><span class="ne-text">武器的发射时机不同，每当为一个块内添加新的实体时，需要刷新 ChunkFragment 中的计时（暂不知道怎么刷新），立马进行一次模拟</span></li><li id="u4a366397" data-lake-index-type="0"><span class="ne-text">对于生命周期短的实体（如子弹），频繁创建与销毁需要不断刷新 ChunkFragment 的计时并且对块内的所有实体进行逻辑更新与执行，可能导致平均检测频率显著提高，脱离本意</span></li></ul><p id="u9dcf1cd7" class="ne-p"><strong><span class="ne-text">方案三：</span></strong></p><p id="ud271f4d5" class="ne-p"><span class="ne-text">通过 FMassTag 标记最新生成的子弹，在 SimulationProcessor 中额外定义一个 EntityQuery 查询附带该 Tag 的子弹 ArcheType 实体，并立马进行一次相同的模拟，然后移除该 Tag ，加入到与已经生成的子弹实体相同 ArcheType 的 Chunk 下统一处理</span></p><p id="udca1a316" class="ne-p"><span class="ne-text">优点：</span></p><ul class="ne-ul"><li id="u917e6a9a" data-lake-index-type="0"><span class="ne-text">不会影响已经生成的子弹的检测频率（计时器），单独处理</span></li></ul><p id="u63945f19" class="ne-p"><span class="ne-text">缺点：</span></p><ul class="ne-ul"><li id="u16c8f518" data-lake-index-type="0"><span class="ne-text">如果子弹频繁生成，那么需要频繁添加和移除 FMassTag 导致 ArcheType 的变化影响内存布局（分配与拷贝），这样带来的开销可能非常大</span></li></ul><p id="u4feec24b" class="ne-p"><strong><span class="ne-text">方案四：</span></strong></p><p id="u9f1fd650" class="ne-p"><span class="ne-text">在 BulletSimulationFragment 中添加一个 bool 值 bNeedFirstSim，表示该子弹实体是否需要第一次模拟。在 SimulationProcessor 中，不根据 ChunkFragment 计时器直接跳过一整个块的模拟，根据设定的检测频率设置 bool ShouldUpdate ，通过遍历块内所有实体，根据其bNeedFirstSim 和 bShouldUpdate 的 || （或值）选择性更新。</span></p><p id="u43125c9c" class="ne-p"><span class="ne-text">优点：</span></p><ul class="ne-ul"><li id="uf18c1cb6" data-lake-index-type="0"><span class="ne-text">对比方案一，bool 内存小于 float，且不用对每个实体计时器进行单独更新</span></li><li id="u03ee91b2" data-lake-index-type="0"><span class="ne-text">对比方案二，不需要刷新和进行块内的全部模拟</span></li><li id="ude62ede1" data-lake-index-type="0"><span class="ne-text">对比方案三，不影响内存布局</span></li></ul><p id="udf74a319" class="ne-p"><span class="ne-text">缺点：</span></p><ul class="ne-ul"><li id="u81c52080" data-lake-index-type="0"><span class="ne-text">需要对每个块的所有实体做遍历判断（包括已经存在的 Chunk，而且其中没有需要更新的子弹实体）</span></li><li id="u8ab90828" data-lake-index-type="0"><span class="ne-text">为 SimulationFragment 添加 bool 值，增加占用</span></li></ul><p id="u39ca5ae3" class="ne-p"><strong><span class="ne-text">方案五：</span></strong></p><p id="ude99103e" class="ne-p"><span class="ne-text">在方案四的基础上，为 BulletSimTimerFragment 添加额外的 bool bIsChunkDirty。通过 ObserverProcessor 设置 EntityAdd 事件，在新的实体添加到某个 Chunk 时，通过BulletSimTimerFragment标记该 Chunk 为脏，那么就需要遍历该 Chunk 内所有实体并对需要执行第一次模拟的实体进行模拟。当遍历完 Chunk 内所有实体后，去除脏标记。在 BulletSimulationProcessor 中就可以直接跳过没有脏标记的 Chunk。</span></p><p id="ueb0c4d24" class="ne-p"><span class="ne-text">优点：</span></p><ul class="ne-ul"><li id="u68011553" data-lake-index-type="0"><span class="ne-text">对比方案四，可以直接跳过不存在新生成实体的 Chunk</span></li></ul><p id="u1d3e04a7" class="ne-p"><span class="ne-text">缺点：</span></p><ul class="ne-ul"><li id="u4d9965bb" data-lake-index-type="0"><span class="ne-text">需要添加额外的判断以修改 ChunkFragment 的 bool 值（当 Chunk 满载且遍历块内实体未发现有需要第一次模拟的实体后，标记该 Chunk 为已满载，后续无需遍历判断。当 Chunk 内子弹实体击中物体后，设置该块为需要遍历）</span></li><li id="ud8facceb" data-lake-index-type="0"><span class="ne-text">ChunkFragment 添加了额外的成员（占用应该可以忽略不记）</span></li></ul><p id="ue81f688d" class="ne-p"><span class="ne-text">关于上述方案的选择，或许需要根据游戏实际场景进行测试与选取。</span></p></details>
<details class="lake-collapse"><summary id="u221cc82f"><span class="ne-text">对于不同来源的相同实体的生成</span></summary><p id="u0c3f117a" class="ne-p"><strong><span class="ne-text">问题：</span></strong></p><p id="u6aff4f3a" class="ne-p"><span class="ne-text">你所说的延迟初始化的思路，我抽象为将调用N次SpawnEntities合并为一次，但是我对于它底层的优化原理还不是特别明白，SpawnEntities的底层是什么。我理解的意思是：在一帧内，有多个不同的地方请求Spawn相同Template的原型，这样会造成内存的切换。但是对于实际场景，不同武器生成的子弹网格体不同，对应的MassEntityConfigAsset也不同吧？但是它们的Fragment组合似乎又相同，那么是同一个ArcheType。我应该有所误解，你详细解释一下吧</span></p><hr id="EzgHU" class="ne-hr"><p id="u4a6c0c17" class="ne-p"><strong><span class="ne-text">Epic 助手解答：</span></strong></p><p id="uc758ecc1" class="ne-p"><span class="ne-text">你的直觉非常敏锐，你已经触及到了 MassEntity 性能优化的核心：</span><strong><span class="ne-text">内存原型（Archetype）的分配逻辑</span></strong><span class="ne-text">。</span></p><p id="u7541db6b" class="ne-p"><span class="ne-text">你提到的疑问主要集中在“网格体不同”与“Fragment 相同”之间的矛盾。让我们深入底层，拆解 </span><code class="ne-code"><span class="ne-text">SpawnEntities</span></code><span class="ne-text"> 的运作机制。</span></p><h3 id="5e832962"><span class="ne-text">1. 为什么“Fragment 相同”不代表“同一个 Archetype”？</span></h3><p id="u85ed2426" class="ne-p"><span class="ne-text">在 MassEntity 中，</span><strong><span class="ne-text">Archetype (原型)</span></strong><span class="ne-text"> 的唯一性不仅取决于 </span><code class="ne-code"><span class="ne-text">FMassFragment</span></code><span class="ne-text">（普通数据碎片），还取决于 </span><strong><span class="ne-text">Shared Fragments (共享碎片)</span></strong><span class="ne-text">。</span></p><ul class="ne-ul"><li id="u22ae46dc" data-lake-index-type="0"><strong><span class="ne-text">普通 Fragment (数据):</span></strong><span class="ne-text"> 如 </span><code class="ne-code"><span class="ne-text">FTransformFragment</span></code><span class="ne-text">。每个实体都有自己独立的一份数据。</span></li><li id="u4df1f970" data-lake-index-type="0"><strong><span class="ne-text">Shared Fragment (共享):</span></strong><span class="ne-text"> 这是一个关键点。诸如“哪个 Static Mesh”、“哪个材质”这类信息，通常被存储在共享碎片中。</span></li></ul><p id="ue292dbda" class="ne-p"><strong><span class="ne-text">底层逻辑：</span></strong><span class="ne-text"> 当你在两个不同的 </span><code class="ne-code"><span class="ne-text">MassEntityConfigAsset</span></code><span class="ne-text"> 中配置了不同的网格体（比如一个是 A 步枪子弹，一个是 B 狙击枪子弹），即便它们的 </span><code class="ne-code"><span class="ne-text">FBulletSimulationFragment</span></code><span class="ne-text"> 定义完全一样，但因为它们指向了不同的 </span><strong><span class="ne-text">Shared Fragments</span></strong><span class="ne-text">（不同的 Mesh 指针），Mass 会为它们创建</span><strong><span class="ne-text">两个不同的 Archetype</span></strong><span class="ne-text">。</span></p><ul class="ne-ul"><li id="u79354620" data-lake-index-type="0"><strong><span class="ne-text">Archetype A:</span></strong><span class="ne-text"> [Transform + SimData + Mesh_A]</span></li><li id="u2f02e774" data-lake-index-type="0"><strong><span class="ne-text">Archetype B:</span></strong><span class="ne-text"> [Transform + SimData + Mesh_B]</span></li></ul><h3 id="230853d7"><span class="ne-text">2. </span><code class="ne-code"><span class="ne-text">SpawnEntities</span></code><span class="ne-text"> 的底层究竟在做什么？</span></h3><p id="u5ed7bdef" class="ne-p"><span class="ne-text">当你调用 </span><code class="ne-code"><span class="ne-text">SpawnEntities</span></code><span class="ne-text"> 时，底层发生了以下步骤：</span></p><ol class="ne-ol"><li id="u2559506d" data-lake-index-type="0"><strong><span class="ne-text">查找 Archetype:</span></strong><span class="ne-text"> 根据 Template 找到对应的内存池。</span></li><li id="ud91efbd5" data-lake-index-type="0"><strong><span class="ne-text">锁定管理器:</span></strong><code class="ne-code"><span class="ne-text">MassEntityManager</span></code><span class="ne-text"> 需要处理内存布局，这是一个线程敏感的操作。</span></li><li id="uc526ada4" data-lake-index-type="0"><strong><span class="ne-text">Chunk 分配:</span></strong><span class="ne-text"> Mass 以 </span><strong><span class="ne-text">Chunk (块)</span></strong><span class="ne-text"> 为单位管理内存（通常每块 64KB）。如果当前 Chunk 满了，它需要申请新内存。</span></li><li id="u3b417621" data-lake-index-type="0"><strong><span class="ne-text">序列化与构造:</span></strong><span class="ne-text"> 在申请到的内存上调用 Fragment 的构造函数。</span></li></ol><h3 id="cfe12502"><span class="ne-text">3. 为什么“合并调用”能优化性能？（核心原理解析）</span></h3><p id="u450a363e" class="ne-p"><span class="ne-text">假设在一帧内，100 个 AI 同时开火（使用相同的子弹配置）：</span></p><ul class="ne-ul"><li id="uc61eb58e" data-lake-index-type="0"><strong><span class="ne-text">方案 A（调用 100 次 SpawnEntities，每次数量 1）：</span></strong></li></ul><ul class="ne-list-wrap"><ul ne-level="1" class="ne-ul"><li id="u104c9cb9" data-lake-index-type="0"><span class="ne-text">底层要执行 100 次内存查找。</span></li><li id="u52bba070" data-lake-index-type="0"><span class="ne-text">要 100 次请求 </span><code class="ne-code"><span class="ne-text">EntityManager</span></code><span class="ne-text"> 的修改权限。</span></li><li id="uda582e58" data-lake-index-type="0"><span class="ne-text">如果 Chunk 恰好在中间满了，会发生多次零散的扩容动作。</span></li><li id="ub0d3d58f" data-lake-index-type="0"><span class="ne-text">最重要的是：这会产生大量的 </span><strong><span class="ne-text">内存零碎操作</span></strong><span class="ne-text">，导致 CPU 指令流水线频繁中断。</span></li></ul></ul><ul class="ne-ul"><li id="ubfde3b44" data-lake-index-type="0"><strong><span class="ne-text">方案 B（合并为 1 次调用，数量 100）：</span></strong></li></ul><ul class="ne-list-wrap"><ul ne-level="1" class="ne-ul"><li id="ud0a47743" data-lake-index-type="0"><strong><span class="ne-text">单次锁定:</span></strong><span class="ne-text"> 只向管理器请求一次修改权限。</span></li><li id="ufa55f7a1" data-lake-index-type="0"><strong><span class="ne-text">批量预留:</span></strong><span class="ne-text"> 管理器一看需要 100 个位置，会一次性计算出需要的空间。如果需要开辟新 Chunk，它会执行一次</span><strong><span class="ne-text">大块内存申请</span></strong><span class="ne-text">。</span></li><li id="u7de40a11" data-lake-index-type="0"><strong><span class="ne-text">缓存友好:</span></strong><span class="ne-text"> CPU 可以利用 SIMD 指令或紧凑的循环，在连续的内存地址上快速写入初始数据（位置、速度）。这被称为 </span><strong><span class="ne-text">线性填充</span></strong><span class="ne-text">，是现代 CPU 最擅长的工作。</span></li></ul></ul><h3 id="fe88f098"><span class="ne-text">4. 纠正误解：不同武器子弹的合并</span></h3><p id="u1dede38a" class="ne-p"><span class="ne-text">你担心的“不同网格体无法合并”是对的。</span></p><ul class="ne-ul"><li id="ud93e3141" data-lake-index-type="0"><strong><span class="ne-text">如果武器 A 和武器 B 子弹配置不同</span></strong><span class="ne-text">（对应不同 ConfigAsset），你</span><strong><span class="ne-text">无法</span></strong><span class="ne-text">通过一次 </span><code class="ne-code"><span class="ne-text">SpawnEntities</span></code><span class="ne-text"> 同时生成它们。你必须分别为每种 ConfigAsset 调用一次。</span></li><li id="uea02e71e" data-lake-index-type="0"><strong><span class="ne-text">真正的优化点在于：</span></strong><span class="ne-text"> 游戏中通常会有大量的同类武器（比如 50 个敌人拿的都是同一种步枪）。将这 50 个人的单发请求合并为一次针对“步枪子弹原型”的批量生成。</span></li></ul><h3 id="3bef4b9c"><span class="ne-text">5. 架构建议：如何处理这种情况？</span></h3><p id="u71a85538" class="ne-p"><span class="ne-text">你可以建立一个简单的 </span><strong><span class="ne-text">BulletSpawnBuffer (子弹生成缓冲)</span></strong><span class="ne-text">：</span></p><ol class="ne-ol"><li id="u91a42001" data-lake-index-type="0"><strong><span class="ne-text">收集阶段：</span></strong><span class="ne-text"> 武器开火时不生成实体，而是将 </span><code class="ne-code"><span class="ne-text">(ConfigAssetPtr, Transform, Velocity)</span></code><span class="ne-text"> 扔进一个 </span><code class="ne-code"><span class="ne-text">TMap&lt;UMassEntityConfigAsset*, TArray&lt;FSpawnRequest&gt;&gt;</span></code><span class="ne-text">。</span></li><li id="uf01150d9" data-lake-index-type="0"><strong><span class="ne-text">生成阶段（每帧末尾）：</span></strong></li></ol><ul class="ne-list-wrap"><ul ne-level="1" class="ne-ul"><li id="u7e0af0d9" data-lake-index-type="0"><span class="ne-text">遍历这个 Map。</span></li><li id="u2ef231da" data-lake-index-type="0"><span class="ne-text">对于每一个 ConfigAsset，获取它的 TemplateID。</span></li><li id="u50368efe" data-lake-index-type="0"><span class="ne-text">调用一次 </span><code class="ne-code"><span class="ne-text">SpawnEntities(TemplateID, Requests.Num(), ...)</span></code><span class="ne-text">。</span></li><li id="ucda975fc" data-lake-index-type="0"><span class="ne-text">使用返回的 Handles，一次性填充所有的初始位置和速度。</span></li></ul></ul><h3 id="8c0a58fe"><span class="ne-text">6. 关于 Elimination (消除) 的补充</span></h3><p id="uc3ee9b17" class="ne-p"><span class="ne-text">这种批处理思维也应应用在销毁上。 在 </span><code class="ne-code"><span class="ne-text">BulletHitProcessor</span></code><span class="ne-text"> 中，你不需要在检测到命中的瞬间立即销毁。你应该把所有命中的 EntityHandle 收集起来，在循环结束后，通过 </span><code class="ne-code"><span class="ne-text">Context.Defer().DestroyEntities(HitHandles)</span></code><span class="ne-text"> 一次性执行。这种批量 </span><strong><span class="ne-text">elimination (消除)</span></strong><span class="ne-text"> 操作比一个一个销毁要快得多，因为它减少了内存块（Chunk）碎片整理的次数。</span></p><p id="uc16a98b1" class="ne-p"><strong><span class="ne-text">总结底层原理：</span></strong><span class="ne-text"> 合并生成的本质是 </span><strong><span class="ne-text">“减少管理开销，换取线性内存写入”</span></strong><span class="ne-text">。虽然不同网格体的子弹必须分批生成，但只要是同一种子弹，合并生成就能极大提升在极端压力下的 CPU 表现。</span></p></details>
<details class="lake-collapse"><summary id="u15cea740"><span class="ne-text">子弹模拟命中实体与画面表现的同步</span></summary><p id="u3198bc62" class="ne-p"><span class="ne-text">主要思路：为 HitFragment 添加一个 float 变量，用于记录当 SimulationProcessor 模拟到子弹命中时，子弹实际飞行到命中点所需的时间。再在 HitFragment 中维护一个 float 变量计时器，然后在 HitProcessor 中，根据 DeltaSecond 对 Timer 进行计时，当达到子弹实际飞行到命中点所需的时间后，再进行伤害的结算和子弹的销毁</span></p></details>
# 测试
## 系统比对
**原先射击系统：**

基于对象池，即时命中的物理模拟射线检测，计算弹道轨迹点数组和子弹飞行时间，传递给子弹对象根据弹道轨迹点数组插值渲染，以及子弹画面表现驱动的伤害应用

缺点：

逻辑存在缺陷，Apex 射击系统并不是即时命中

子弹网格体驱动伤害应用

存在大量子弹时，对象维护成本及占用高



**改进后射击系统：**

基于 MassEntity 系统的数据驱动的子弹系统，每帧检测

## 游戏内测试
**测试背景：**

 20 把武器，以一定频率向天空进行射击，最高同时存在 540 个子弹实体

**MassDebugger**

<!-- 这是一张图片，ocr 内容为： -->
![](https://cdn.nlark.com/yuque/0/2026/png/63011593/1782458299146-e787183a-55a9-4eb4-9cfd-bd21f99db8e7.png)

**基于对象池的子弹系统测试表现**

<!-- 这是一张图片，ocr 内容为： -->
![](https://cdn.nlark.com/yuque/0/2026/png/63011593/1782457412741-78c99b5b-6bca-4360-9e1a-dbf13412111e.png)

平均帧时间为 12ms

平均帧数 80 左右波动

**基于 MassEntity 的子弹系统测试表现**

<!-- 这是一张图片，ocr 内容为： -->
![](https://cdn.nlark.com/yuque/0/2026/png/63011593/1782459408733-e0abdf4e-dd7a-4c84-9b43-0dfb264f02aa.png)

平均帧时间 11ms

平均帧数 90 左右波动

**结论**

基于 MassEntity 的子弹系统，在巨量实体模拟的情况下，要显著由于以对象为

