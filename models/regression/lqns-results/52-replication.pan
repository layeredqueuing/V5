<?xml version="1.0"?>
<!-- lqns -pragma=variance=mol,threads=hyper -pragma=replication=pan -xml -output=52-replication-pan.lqxo -->
<lqn-model name="52-replication" description="lqns 5.28.4 solution for model from: 52-replication.lqnx." xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="/usr/local/share/lqns/lqn.xsd">
  <solver-params comment="Simplified model from bug 166, Prune replication" conv_val="0.01" it_limit="75" underrelax_coeff="0.9" print_int="1">
    <pragma param="replication" value="pan"/>
    <pragma param="threads" value="hyper"/>
    <pragma param="variance" value="mol"/>
    <result-general solver-info="lqns 5.28.4" valid="true" conv-val="0.00391427" iterations="2" platform-info="Gregs-Retina-iMac.local Darwin 22.3.0" user-cpu-time=" 0:00:00.000" system-cpu-time=" 0:00:00.000" elapsed-time=" 0:00:00.000" max-rss="2420736">
      <mva-info submodels="2" core="6" step="35" step-squared="205" wait="543" wait-squared="50985" faults="0"/>
    </result-general>
  </solver-params>
  <processor name="p1" scheduling="inf">
    <result-processor utilization="0.333588"/>
    <task name="t1" scheduling="ref">
      <result-task throughput="0.333588" utilization="1.00077" phase1-utilization="1.00077" proc-utilization="0.333588"/>
      <fan-out dest="t2" value="2"/>
      <entry name="e1" type="PH1PH2">
        <result-entry utilization="1.00077" throughput="0.333588" proc-utilization="0.333588" squared-coeff-variation="1.51852" throughput-bound="0.333333"/>
        <entry-phase-activities>
          <activity name="e1_1" phase="1" host-demand-mean="1">
            <result-activity proc-waiting="0" service-time="3" utilization="1.00077" service-time-variance="13.6667"/>
            <synch-call dest="e2" calls-mean="1">
              <result-call waiting="0"/>
            </synch-call>
          </activity>
        </entry-phase-activities>
      </entry>
    </task>
  </processor>
  <processor name="p2" scheduling="ps" quantum="0.2" replication="2">
    <result-processor utilization="0.333588"/>
    <task name="t2" scheduling="fcfs" replication="2">
      <result-task throughput="0.333588" utilization="0.333588" phase1-utilization="0.333588" proc-utilization="0.333588"/>
      <entry name="e2" type="PH1PH2">
        <result-entry utilization="0.333588" throughput="0.333588" proc-utilization="0.333588" squared-coeff-variation="1" throughput-bound="1"/>
        <entry-phase-activities>
          <activity name="e2_1" phase="1" host-demand-mean="1">
            <result-activity proc-waiting="0" service-time="1" utilization="0.333588" service-time-variance="1"/>
          </activity>
        </entry-phase-activities>
      </entry>
    </task>
  </processor>
</lqn-model>
