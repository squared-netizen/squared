#include <squared/scene2d/stage.hpp>

#include <cassert>
#include <memory>
#include <vector>

namespace {

class RecordingActor final : public squared::scene2d::Actor {
public:
    RecordingActor(int value, std::vector<int>& order)
        : value_(value), order_(order) {}

    void act(double delta_seconds) override
    {
        assert(delta_seconds == 0.25);
        order_.push_back(value_);
    }

private:
    int value_;
    std::vector<int>& order_;
};

} // namespace

int main()
{
    using squared::scene2d::Actor;
    using squared::scene2d::Group;
    using squared::scene2d::Stage;

    std::vector<int> order;
    Stage stage(320.0F, 180.0F);

    auto first = std::make_unique<RecordingActor>(1, order);
    first->set_bounds(10.0F, 20.0F, 80.0F, 60.0F);
    Actor* first_pointer = first.get();
    Actor& first_reference = stage.add_actor(std::move(first));
    assert(&first_reference == first_pointer);
    assert(first_reference.parent() == &stage.root());

    auto group = std::make_unique<Group>();
    group->set_bounds(10.0F, 20.0F, 80.0F, 60.0F);
    Group* group_pointer = group.get();
    Actor& group_reference = stage.add_actor(std::move(group));
    assert(&group_reference == group_pointer);

    auto second = std::make_unique<RecordingActor>(2, order);
    second->set_bounds(0.0F, 0.0F, 80.0F, 60.0F);
    Actor* second_pointer = second.get();
    [[maybe_unused]] Actor& second_reference =
        group_pointer->add_actor(std::move(second));

    stage.act(0.25);
    assert((order == std::vector<int>{1, 2}));

    // The later group is topmost and its child receives local coordinates.
    assert(stage.hit(20.0F, 30.0F) == second_pointer);
    second_pointer->set_touchable(false);
    assert(stage.hit(20.0F, 30.0F) == group_pointer);
    assert(stage.hit(20.0F, 30.0F, false) == second_pointer);
    group_pointer->set_visible(false);
    assert(stage.hit(20.0F, 30.0F) == first_pointer);
    assert(stage.hit(500.0F, 500.0F) == nullptr);

    std::unique_ptr<Actor> removed =
        stage.root().remove_actor(first_reference);
    assert(removed.get() == first_pointer);
    assert(removed->parent() == nullptr);
    assert(stage.root().child_count() == 1);
    assert(stage.root().child_at(0) == group_pointer);
    assert(stage.root().child_at(1) == nullptr);

    stage.resize(-1.0F, 90.0F);
    assert(stage.root().width() == 0.0F);
    assert(stage.root().height() == 90.0F);
    stage.root().clear();
    assert(stage.root().child_count() == 0);
}
