<?xml version="1.0"?>
<!-- lqns -pragma=variance=mol,threads=hyper -no-warnings -pragma=replication=pan -xml -output=55-replication.pan -->
<lqn-model name="55-replication" description="lqns 5.32 solution for 55-replication.lqnx." xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="/usr/local/share/lqns/lqn.xsd">
  <solver-params comment="fork-join (set1) with Prune replication" conv_val="1e-06" it_limit="50" underrelax_coeff="0.9" print_int="5">
    <pragma param="replication" value="pan"/>
    <pragma param="severity-level" value="run-time"/>
    <pragma param="threads" value="hyper"/>
    <pragma param="variance" value="mol"/>
    <result-general solver-info="lqns 5.32" valid="true" conv-val="2.52954e-07" iterations="7" platform-info="Mac Darwin 24.6.0" user-cpu-time=" 0:00:00.001" system-cpu-time=" 0:00:00.000" elapsed-time=" 0:00:00.001" max-rss="7094272">
      <mva-info submodels="3" core="33" step="263" step-squared="3059" wait="25998" wait-squared="4.61036e+07" faults="0"/>
    </result-general>
  </solver-params>
  <processor name="d1" scheduling="fcfs" replication="4">
    <result-processor utilization="0.225894"/>
    <task name="d1" scheduling="fcfs" replication="4">
      <result-task throughput="5.64736" utilization="0.225894" phase1-utilization="0.225894" proc-utilization="0.225894"/>
      <entry name="d1" type="PH1PH2">
        <result-entry utilization="0.225894" throughput="5.64736" proc-utilization="0.225894" squared-coeff-variation="1" throughput-bound="25"/>
        <entry-phase-activities>
          <activity name="d1_ph1" phase="1" host-demand-mean="0.04">
            <result-activity proc-waiting="0" service-time="0.04" utilization="0.225894" service-time-variance="0.0016"/>
          </activity>
        </entry-phase-activities>
      </entry>
    </task>
  </processor>
  <processor name="p1" scheduling="fcfs">
    <result-processor utilization="0.225894"/>
    <task name="t1" scheduling="ref">
      <result-task throughput="0.752981" utilization="1" phase1-utilization="1" proc-utilization="0.225894"/>
      <fan-out dest="d1" value="4"/>
      <entry name="e1" type="NONE">
        <result-entry utilization="1" throughput="0.752981" proc-utilization="0" squared-coeff-variation="0.379658" throughput-bound="2" phase1-service-time="1.32805" phase1-service-time-variance="0.669615" phase1-utilization="1"/>
      </entry>
      <task-activities>
        <activity name="a1" bound-to-entry="e1" host-demand-mean="0.1">
          <result-activity proc-waiting="0" service-time="0.5" utilization="0.37649" service-time-variance="0.283455" throughput="0.752981" proc-utilization="0.0752981"/>
          <synch-call dest="d1" calls-mean="2.5">
            <result-call waiting="0"/>
          </synch-call>
        </activity>
        <activity name="b1" host-demand-mean="0.1">
          <result-activity proc-waiting="0.00892598" service-time="0.54463" utilization="0.410096" service-time-variance="0.336153" throughput="0.752981" proc-utilization="0.0752981"/>
          <synch-call dest="d1" calls-mean="2.5">
            <result-call waiting="0.00357039"/>
          </synch-call>
        </activity>
        <activity name="b2" host-demand-mean="0.1">
          <result-activity proc-waiting="0.00892598" service-time="0.54463" utilization="0.410096" service-time-variance="0.336153" throughput="0.752981" proc-utilization="0.0752981"/>
          <synch-call dest="d1" calls-mean="2.5">
            <result-call waiting="0.00357039"/>
          </synch-call>
        </activity>
        <activity name="c1" host-demand-mean="0">
          <result-activity proc-waiting="0" service-time="0" utilization="0" throughput="0.752981" proc-utilization="0"/>
        </activity>
        <precedence>
          <pre>
            <activity name="a1"/>
          </pre>
          <post-AND>
            <activity name="b1"/>
            <activity name="b2"/>
          </post-AND>
        </precedence>
        <precedence>
          <pre-AND>
            <result-join-delay join-waiting="0.828055" join-variance="0.38616"/>
            <activity name="b1"/>
            <activity name="b2"/>
          </pre-AND>
          <post>
            <activity name="c1"/>
          </post>
        </precedence>
      </task-activities>
    </task>
  </processor>
</lqn-model>
