<?xml version="1.0"?>
<!-- lqns -pragma=variance=mol,threads=hyper -no-warnings -pragma=replication=pan -xml -output=51-replication.pan -->
<lqn-model name="51-replication" description="lqns 5.32 solution for 51-replication.lqnx." xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="/usr/local/share/lqns/lqn.xsd">
  <solver-params comment="Pan thesis, case 3." conv_val="1e-05" it_limit="500" underrelax_coeff="0.9" print_int="10">
    <pragma param="replication" value="pan"/>
    <pragma param="severity-level" value="run-time"/>
    <pragma param="threads" value="hyper"/>
    <pragma param="variance" value="mol"/>
    <result-general solver-info="lqns 5.32" valid="true" conv-val="1.6293e-06" iterations="2" platform-info="Mac Darwin 24.6.0" user-cpu-time=" 0:00:00.027" system-cpu-time=" 0:00:00.000" elapsed-time=" 0:00:00.027" max-rss="6963200">
      <mva-info submodels="2" core="31" step="1742" step-squared="150416" wait="4.5092e+06" wait-squared="1.51733e+12" faults="0"/>
    </result-general>
  </solver-params>
  <processor name="A1" scheduling="fcfs" replication="2">
    <result-processor utilization="0.0149589"/>
    <task name="A1" scheduling="ref" replication="2">
      <result-task throughput="0.00747943" utilization="1" phase2-utilization="1" proc-utilization="0.0149589"/>
      <fan-out dest="C1" value="3"/>
      <entry name="A1" type="PH1PH2">
        <result-entry utilization="1" throughput="0.00747943" proc-utilization="0.0149589" squared-coeff-variation="1.16794" throughput-bound="0.0263158"/>
        <entry-phase-activities>
          <activity name="A1_ph2" phase="2" host-demand-mean="2">
            <result-activity proc-waiting="0" service-time="133.7" utilization="1" service-time-variance="20877.8"/>
            <synch-call dest="C1" calls-mean="2">
              <result-call waiting="7.97503"/>
            </synch-call>
            <synch-call dest="C2" calls-mean="2">
              <result-call waiting="7.97503"/>
            </synch-call>
          </activity>
        </entry-phase-activities>
      </entry>
    </task>
  </processor>
  <processor name="B1" scheduling="fcfs" replication="10">
    <result-processor utilization="0.0161225"/>
    <task name="B1" scheduling="ref" replication="10">
      <result-task throughput="0.00403063" utilization="1" phase2-utilization="1" proc-utilization="0.0161225"/>
      <fan-out dest="C1" value="3"/>
      <entry name="B1" type="PH1PH2">
        <result-entry utilization="1" throughput="0.00403063" proc-utilization="0.0161225" squared-coeff-variation="1.08307" throughput-bound="0.0102041"/>
        <entry-phase-activities>
          <activity name="B1_ph2" phase="2" host-demand-mean="4">
            <result-activity proc-waiting="0" service-time="248.101" utilization="1" service-time-variance="66667.6"/>
            <synch-call dest="C1" calls-mean="3">
              <result-call waiting="8.33894"/>
            </synch-call>
            <synch-call dest="C2" calls-mean="3">
              <result-call waiting="8.33894"/>
            </synch-call>
            <synch-call dest="D1" calls-mean="4">
              <result-call waiting="0"/>
            </synch-call>
            <synch-call dest="D2" calls-mean="4">
              <result-call waiting="0"/>
            </synch-call>
          </activity>
        </entry-phase-activities>
      </entry>
    </task>
  </processor>
  <processor name="C1" scheduling="fcfs" replication="3">
    <result-processor utilization="0.905018"/>
    <task name="C1" scheduling="fcfs" replication="3">
      <result-task throughput="0.301673" utilization="0.905018" phase1-utilization="0.905018" proc-utilization="0.905018"/>
      <fan-in source="A1" value="2"/>
      <fan-in source="B1" value="10"/>
      <entry name="C1" type="PH1PH2">
        <result-entry utilization="0.452509" throughput="0.150836" proc-utilization="0.452509" squared-coeff-variation="1" throughput-bound="0.333333"/>
        <entry-phase-activities>
          <activity name="C1_ph1" phase="1" host-demand-mean="3">
            <result-activity proc-waiting="0" service-time="3" utilization="0.452509" service-time-variance="9"/>
          </activity>
        </entry-phase-activities>
      </entry>
      <entry name="C2" type="PH1PH2">
        <result-entry utilization="0.452509" throughput="0.150836" proc-utilization="0.452509" squared-coeff-variation="1" throughput-bound="0.333333"/>
        <entry-phase-activities>
          <activity name="C2_ph1" phase="1" host-demand-mean="3">
            <result-activity proc-waiting="0" service-time="3" utilization="0.452509" service-time-variance="9"/>
          </activity>
        </entry-phase-activities>
      </entry>
    </task>
  </processor>
  <processor name="D1" scheduling="fcfs" replication="10">
    <result-processor utilization="0.161225"/>
    <task name="D1" scheduling="fcfs" replication="10">
      <result-task throughput="0.032245" utilization="0.161225" phase1-utilization="0.161225" proc-utilization="0.161225"/>
      <entry name="D1" type="PH1PH2">
        <result-entry utilization="0.0806125" throughput="0.0161225" proc-utilization="0.0806125" squared-coeff-variation="1" throughput-bound="0.2"/>
        <entry-phase-activities>
          <activity name="D1_ph1" phase="1" host-demand-mean="5">
            <result-activity proc-waiting="0" service-time="5" utilization="0.0806125" service-time-variance="25"/>
          </activity>
        </entry-phase-activities>
      </entry>
      <entry name="D2" type="PH1PH2">
        <result-entry utilization="0.0806125" throughput="0.0161225" proc-utilization="0.0806125" squared-coeff-variation="1" throughput-bound="0.2"/>
        <entry-phase-activities>
          <activity name="D2_ph1" phase="1" host-demand-mean="5">
            <result-activity proc-waiting="0" service-time="5" utilization="0.0806125" service-time-variance="25"/>
          </activity>
        </entry-phase-activities>
      </entry>
    </task>
  </processor>
</lqn-model>
