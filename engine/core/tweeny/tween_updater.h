#pragma once
#include "tween_action.h"
#include "tween_private.h"
#include "tween_ease.h"
#include "tween_math.h"

namespace tweeny
{

template<typename Object,
         typename TargetType,
         typename InitializeFunc,
         typename UpdateFunc,
         typename Getter>
auto create_tween_updater(Object* object,
                     const TargetType& end,
					 const sentinel_t& sentinel,
                     InitializeFunc&& initialize_func,
                     UpdateFunc&& update_func,
                     Getter&& getter,
                     const ease_t& ease_func = ease::linear,
                     const interpolate_t<TargetType>& interpolate = lerp<TargetType>)
{
    auto updater = [object
            , begin = TargetType{}
            , prev = TargetType{}
            , end = end
			, sentinel = sentinel
            , initialize_func = std::forward<InitializeFunc>(initialize_func)
            , update_func = std::forward<UpdateFunc>(update_func)
            , getter = std::forward<Getter>(getter)
            , ease_func = ease_func
            , interpolate = interpolate
            , start_required = true
            , finished = false]
    (duration_t delta, tween_action& self) mutable -> state_t
    {
        if(finished)
        {
            return state_t::finished;
        }

        if(sentinel.expired())
        {
            finished = true;
            return state_t::finished;
        }

        if(tween_private::get_state(self) == state_t::finished)
        {
            finished = true;
            return state_t::finished;
        }

        if(tween_private::get_state(self) == state_t::paused)
        {
            return state_t::paused;
        }

        const auto duration = tween_private::get_duration(self);

        if(duration == duration_t::zero())
        {
            auto old_object = getter(object, self);
            bool request_on_step = old_object != end;
            update_func(object, end, self);

            if(request_on_step)
            {
                self.on_step.emit();
            }
            if(!tween_private::is_stop_and_finished_requested(self))
            {
                if(tween_private::get_state(self) == state_t::paused)
                {
                    return state_t::paused;
                }
            }
            finished = true;
            return state_t::finished;
        }

        if(start_required)
        {
            auto old_object = getter(object, self);
            begin = initialize_func(object, sentinel, self);
            start_required = false;
            auto updated_object = getter(object, self);
            if(old_object != updated_object)
            {
                self.on_step.emit();
            }
            if(!tween_private::is_stop_and_finished_requested(self))
            {
                if(tween_private::get_state(self) == state_t::paused)
                {
                    return state_t::paused;
                }
            }
            return state_t::running;
        }

        tween_private::update_elapsed(self, delta);
        const auto elapsed = tween_private::get_elapsed(self);

        const float progress = float(elapsed.count()) / float(duration.count());
        const auto next = interpolate(begin, end, progress, ease_func);

        if( prev != next || next == end)
        {
            update_func(object, next, self);
            self.on_step.emit();
            prev = next;
        }

        if(!tween_private::is_stop_and_finished_requested(self))
        {
            if(tween_private::get_state(self) == state_t::paused)
            {
                return state_t::paused;
            }
        }

        if(elapsed == duration)
        {
            finished = true;
            return state_t::finished;
        }

        if(tween_private::get_state(self) == state_t::finished)
        {
            finished = true;
            return state_t::finished;
        }

        return state_t::running;
    };

    return updater;
}

}