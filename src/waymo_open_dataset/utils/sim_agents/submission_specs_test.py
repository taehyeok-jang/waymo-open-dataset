# Copyright 2023 The Waymo Open Dataset Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# =============================================================================
"""Tests for waymo_open_dataset.utils.sim_agents.submission_specs."""

from absl.testing import parameterized
import tensorflow as tf

from waymo_open_dataset.protos import scenario_pb2
from waymo_open_dataset.protos import sim_agents_submission_pb2
from waymo_open_dataset.utils import test_utils
from waymo_open_dataset.utils.sim_agents import submission_specs
from waymo_open_dataset.utils.sim_agents import test_utils as sim_agents_test_utils


class SubmissionSpecsTest(tf.test.TestCase, parameterized.TestCase):

  @parameterized.parameters(
      (submission_specs.ChallengeType.SIM_AGENTS, 83, 50),
      (submission_specs.ChallengeType.SCENARIO_GEN, 83, 50),
  )
  def test_is_valid_sim_agent(
      self,
      challenge_type: submission_specs.ChallengeType,
      expected_n_objects: int,
      expected_n_sim_agents: int,
  ):
    scenario = test_utils.get_womd_test_scenario()
    # Track 0 is a valid sim agent because it's valid at step 11 (1-indexed).
    valid_track = scenario.tracks[0]
    submission_config = submission_specs.get_submission_config(challenge_type)
    self.assertTrue(submission_config.is_valid_sim_agent(valid_track))
    # Track 64 is an invalid sim agent because it's invalid at step 11.
    invalid_track = scenario.tracks[64]
    self.assertFalse(submission_config.is_valid_sim_agent(invalid_track))
    valid_mask = [
        submission_config.is_valid_sim_agent(track) for track in scenario.tracks
    ]
    self.assertLen(valid_mask, expected_n_objects)
    self.assertEqual(sum(valid_mask), expected_n_sim_agents)

  @parameterized.parameters(
      (submission_specs.ChallengeType.SIM_AGENTS, 50),
      (submission_specs.ChallengeType.SCENARIO_GEN, 50),
  )
  def test_get_sim_agent_ids(
      self,
      challenge_type: submission_specs.ChallengeType,
      expected_n_sim_agents: int,
  ):
    scenario = test_utils.get_womd_test_scenario()
    sim_agent_ids = submission_specs.get_sim_agent_ids(scenario, challenge_type)
    self.assertLen(sim_agent_ids, expected_n_sim_agents)

  @parameterized.parameters(
      (submission_specs.ChallengeType.SIM_AGENTS, 4),
      (submission_specs.ChallengeType.SCENARIO_GEN, 50),
  )
  def test_get_evaluation_sim_agent_ids(
      self,
      challenge_type: submission_specs.ChallengeType,
      expected_n_eval_sim_agents: int,
  ):
    scenario = test_utils.get_womd_test_scenario()
    eval_sim_agent_ids = submission_specs.get_evaluation_sim_agent_ids(
        scenario, challenge_type
    )
    self.assertLen(eval_sim_agent_ids, expected_n_eval_sim_agents)

  @parameterized.parameters(
      (submission_specs.ChallengeType.SIM_AGENTS),
      (submission_specs.ChallengeType.SCENARIO_GEN),
  )
  def test_get_evaluation_sim_agent_ids_no_repetitions(
      self, challenge_type: submission_specs.ChallengeType
  ):
    scenario = test_utils.get_womd_test_scenario()
    # Let's modify the scenario such that the AV is inside `tracks_to_predict`
    # (as this happens inconsistently in the dataset).
    scenario.tracks_to_predict.append(
        scenario_pb2.RequiredPrediction(track_index=scenario.sdc_track_index)
    )
    eval_sim_agent_ids = submission_specs.get_evaluation_sim_agent_ids(
        scenario, challenge_type
    )
    self.assertLen(eval_sim_agent_ids, len(set(eval_sim_agent_ids)))

  @parameterized.parameters(
      (submission_specs.ChallengeType.SIM_AGENTS),
      (submission_specs.ChallengeType.SCENARIO_GEN),
  )
  def test_validate_joint_scene_success(
      self, challenge_type: submission_specs.ChallengeType
  ):
    scenario = test_utils.get_womd_test_scenario()
    config = submission_specs.get_submission_config(challenge_type)
    # Create a joint scene with the correct objects and the correct length.
    trajectories = sim_agents_test_utils.get_test_simulated_trajectories(
        scenario,
        challenge_type=challenge_type,
        num_sim_steps=config.n_simulation_steps,
    )
    joint_scene = sim_agents_submission_pb2.JointScene(
        simulated_trajectories=trajectories
    )
    submission_specs.validate_joint_scene(joint_scene, scenario, challenge_type)

  @parameterized.parameters(
      (submission_specs.ChallengeType.SIM_AGENTS),
      (submission_specs.ChallengeType.SCENARIO_GEN),
  )
  def test_validate_joint_scene_fails_on_wrong_length(
      self, challenge_type: submission_specs.ChallengeType
  ):
    scenario = test_utils.get_womd_test_scenario()
    # Create a joint scene with the correct objects and the wrong length.
    trajectories = sim_agents_test_utils.get_test_simulated_trajectories(
        scenario, challenge_type, num_sim_steps=42
    )
    joint_scene = sim_agents_submission_pb2.JointScene(
        simulated_trajectories=trajectories
    )
    with self.assertRaises(ValueError):
      submission_specs.validate_joint_scene(
          joint_scene, scenario, challenge_type
      )

  @parameterized.parameters(
      (submission_specs.ChallengeType.SIM_AGENTS),
      (submission_specs.ChallengeType.SCENARIO_GEN),
  )
  def test_validate_joint_scene_fails_on_missing_objects(
      self, challenge_type: submission_specs.ChallengeType
  ):
    config = submission_specs.get_submission_config(challenge_type)
    scenario = test_utils.get_womd_test_scenario()
    # Create a joint scene with the correct length but one missing object.
    trajectories = sim_agents_test_utils.get_test_simulated_trajectories(
        scenario, challenge_type, num_sim_steps=config.n_simulation_steps
    )
    joint_scene = sim_agents_submission_pb2.JointScene(
        simulated_trajectories=trajectories[:-1]
    )
    with self.assertRaises(ValueError):
      submission_specs.validate_joint_scene(
          joint_scene, scenario, challenge_type
      )

  @parameterized.parameters(
      (submission_specs.ChallengeType.SIM_AGENTS),
      (submission_specs.ChallengeType.SCENARIO_GEN),
  )
  def test_validate_joint_scene_fails_on_wrong_objects(
      self, challenge_type: submission_specs.ChallengeType
  ):
    scenario = test_utils.get_womd_test_scenario()
    config = submission_specs.get_submission_config(challenge_type)
    # Create a joint scene with the correct length but one additional object.
    trajectories = sim_agents_test_utils.get_test_simulated_trajectories(
        scenario, challenge_type, num_sim_steps=config.n_simulation_steps
    )
    trajectories.append(
        sim_agents_submission_pb2.SimulatedTrajectory(
            center_x=[0.0] * config.n_simulation_steps,
            center_y=[0.0] * config.n_simulation_steps,
            center_z=[0.0] * config.n_simulation_steps,
            heading=[0.0] * config.n_simulation_steps,
            object_id=0,
        )
    )
    joint_scene = sim_agents_submission_pb2.JointScene(
        simulated_trajectories=trajectories
    )
    with self.assertRaises(ValueError):
      submission_specs.validate_joint_scene(
          joint_scene, scenario, challenge_type
      )

  @parameterized.parameters(
      (submission_specs.ChallengeType.SIM_AGENTS),
      (submission_specs.ChallengeType.SCENARIO_GEN),
  )
  def test_validate_scenario_rollouts_correct_n_simulations(
      self, challenge_type: submission_specs.ChallengeType
  ):
    config = submission_specs.get_submission_config(challenge_type)
    scenario = test_utils.get_womd_test_scenario()
    # Create a joint scene with the correct objects and the correct length.
    trajectories = sim_agents_test_utils.get_test_simulated_trajectories(
        scenario, challenge_type, num_sim_steps=config.n_simulation_steps
    )
    joint_scene = sim_agents_submission_pb2.JointScene(
        simulated_trajectories=trajectories
    )
    scenario_rollouts = sim_agents_submission_pb2.ScenarioRollouts(
        joint_scenes=[joint_scene] * config.n_rollouts,
        scenario_id=scenario.scenario_id,
    )
    submission_specs.validate_scenario_rollouts(
        scenario_rollouts, scenario, challenge_type
    )

  @parameterized.parameters(
      (submission_specs.ChallengeType.SIM_AGENTS),
      (submission_specs.ChallengeType.SCENARIO_GEN),
  )
  def test_validate_scenario_rollouts_missing_id(
      self, challenge_type: submission_specs.ChallengeType
  ):
    config = submission_specs.get_submission_config(challenge_type)
    scenario = test_utils.get_womd_test_scenario()
    # Create a joint scene with the correct objects and the correct length.
    trajectories = sim_agents_test_utils.get_test_simulated_trajectories(
        scenario, challenge_type, num_sim_steps=config.n_simulation_steps
    )
    joint_scene = sim_agents_submission_pb2.JointScene(
        simulated_trajectories=trajectories
    )
    scenario_rollouts = sim_agents_submission_pb2.ScenarioRollouts(
        joint_scenes=[joint_scene] * config.n_rollouts
    )
    with self.assertRaises(ValueError):
      submission_specs.validate_scenario_rollouts(
          scenario_rollouts, scenario, challenge_type
      )

  @parameterized.parameters(
      (submission_specs.ChallengeType.SIM_AGENTS,),
  )
  def test_validate_scenario_rollouts_wrong_n_simulations(
      self, challenge_type: submission_specs.ChallengeType
  ):
    config = submission_specs.get_submission_config(challenge_type)
    scenario = test_utils.get_womd_test_scenario()
    # Create a joint scene with the correct objects and the correct length.
    trajectories = sim_agents_test_utils.get_test_simulated_trajectories(
        scenario, challenge_type, num_sim_steps=config.n_simulation_steps
    )
    joint_scene = sim_agents_submission_pb2.JointScene(
        simulated_trajectories=trajectories
    )
    scenario_rollouts = sim_agents_submission_pb2.ScenarioRollouts(
        joint_scenes=[joint_scene] * 24, scenario_id=scenario.scenario_id
    )
    with self.assertRaises(ValueError):
      submission_specs.validate_scenario_rollouts(
          scenario_rollouts, scenario, challenge_type
      )

  def test_validate_scenario_with_test_submission(self):
    scenario = test_utils.get_womd_test_scenario()
    test_submission = sim_agents_test_utils.load_test_submission()
    scenario_rollouts = test_submission.scenario_rollouts[0]
    submission_specs.validate_scenario_rollouts(
        scenario_rollouts,
        scenario,
        challenge_type=submission_specs.ChallengeType.SIM_AGENTS,
    )


if __name__ == '__main__':
  tf.random.set_seed(42)
  tf.test.main()
