#!/usr/bin/env python3

import json
import sys
from dataclasses import dataclass
import matplotlib.pyplot as plt
import numpy as np


@dataclass
class Delta:
    start_secs: float
    end_secs: float

    def delta_millis(self):
        return (self.end_secs - self.start_secs) * 1000.0

    def geometric_mean_secs(self):
        return (self.end_secs + self.start_secs) / 2.0


def tick_to_second(ticks):
    return float(ticks) / 10000.0


def second_to_tick(seconds):
    return seconds * 10000.0


def process_data(data, start_at_sec, max_duration_secs):
    times_per_buffer = []

    for start_tick, end_tick in data["per_buffer"]:
        # ignore setup time
        if tick_to_second(end_tick) < start_at_sec:
            continue

        # discard values we do not want to plot
        if tick_to_second(start_tick) > start_at_sec + max_duration_secs:
            break

        times_per_buffer.append(
            Delta(
                start_secs=max(tick_to_second(start_tick), start_at_sec),
                end_secs=min(
                    tick_to_second(end_tick), start_at_sec + max_duration_secs
                ),
            )
        )

    times_between_buffers = []
    between_buffer_pairs = zip([0.0] + data["between_buffers"], data["between_buffers"])

    for prev_start_tick, start_tick in between_buffer_pairs:
        # ignore setup time
        if tick_to_second(start_tick) < start_at_sec:
            continue

        # discard values we do not want to plot
        if tick_to_second(prev_start_tick) > start_at_sec + max_duration_secs:
            break

        times_between_buffers.append(
            Delta(
                start_secs=max(tick_to_second(prev_start_tick), start_at_sec),
                end_secs=min(
                    tick_to_second(start_tick), start_at_sec + max_duration_secs
                ),
            )
        )

    return (times_per_buffer, times_between_buffers)


plot_type = sys.argv[1]
json_path = sys.argv[2]

with open(json_path, "r") as json_file:
    data = json.load(json_file)

times_per_buffer, times_between_buffers = process_data(
    data, start_at_sec=20.0, max_duration_secs=60.0
)

if plot_type == "per-buffer-scatter":
    plt.scatter(
        [delta.geometric_mean_secs() for delta in times_per_buffer],
        [delta.delta_millis() for delta in times_per_buffer],
        marker=".",
        color="black",
    )

    deltas = np.array([delta.delta_millis() for delta in times_per_buffer])
    plt.axhline(np.max(deltas), label="max", color="red")
    plt.axhline(
        np.median(deltas), label="median", color="cornflowerblue", linestyle="-"
    )
    plt.axhline(np.mean(deltas), label="mean", color="cornflowerblue", linestyle="--")
    plt.axhline(np.min(deltas), label="min", color="green")

    plt.xlabel("time (s)")
    plt.ylabel("time per buffer (ms)")
    plt.legend(loc="lower right")
    plt.show()
elif plot_type == "between-buffers-scatter":
    plt.scatter(
        [delta.geometric_mean_secs() for delta in times_between_buffers],
        [delta.delta_millis() for delta in times_between_buffers],
        marker=".",
        color="black",
    )

    deltas = np.array([delta.delta_millis() for delta in times_between_buffers])
    plt.axhline(np.max(deltas), label="max", color="red")
    plt.axhline(
        np.median(deltas), label="median", color="cornflowerblue", linestyle="-"
    )
    plt.axhline(np.mean(deltas), label="mean", color="cornflowerblue", linestyle="--")
    plt.axhline(np.min(deltas), label="min", color="green")

    plt.xlabel("time (s)")
    plt.ylabel("time between buffers (ms)")
    plt.legend(loc="lower right")
    plt.show()
elif plot_type == "between-buffers-hist":
    deltas = np.array([delta.delta_millis() for delta in times_between_buffers])
    plt.hist(deltas, color="green")

    plt.text(
        0.95,
        0.95,
        f"total count: {len(deltas)}",
        verticalalignment="top",
        horizontalalignment="right",
        transform=plt.gca().transAxes,
    )

    plt.xlabel("time between buffers (ms)")
    plt.ylabel("count")
    plt.show()
else:
    sys.stderr.write(f"unknown plot type `{plot_type}`\n")
    exit(1)
